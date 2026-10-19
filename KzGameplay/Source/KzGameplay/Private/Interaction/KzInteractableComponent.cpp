// Copyright 2026 kirzo

#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractableInterface.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	using FActionKey = TPair<TWeakObjectPtr<UKzInteractorComponent>, FGameplayTag>;
}

void KzInteraction::DeclareContext(FScriptableContainer& Container)
{
	Container.AddContextProperty<AActor*>(TEXT("Instigator"));
	Container.AddContextProperty<UKzInteractorComponent*>(TEXT("Interactor"));
	Container.AddContextProperty<AActor*>(TEXT("Owner"));
	Container.AddContextProperty<UKzInteractableComponent*>(TEXT("Interactable"));
}

void KzInteraction::FillContext(FScriptableContainer& Container, UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	Container.SetContextProperty(TEXT("Instigator"), Interactor ? Interactor->GetOwner() : nullptr);
	Container.SetContextProperty(TEXT("Interactor"), Interactor);
	Container.SetContextProperty(TEXT("Owner"), Interactable ? Interactable->GetOwner() : nullptr);
	Container.SetContextProperty(TEXT("Interactable"), Interactable);
}

FKzInteractionAction::FKzInteractionAction()
{
	KzInteraction::DeclareContext(Requirement);
	KzInteraction::DeclareContext(Effect);
}

UKzInteractableComponent::UKzInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	KzInteraction::DeclareContext(InteractionRequirement);
	KzInteraction::DeclareContext(AvailabilityRequirement);
	KzInteraction::DeclareContext(InteractionAction);
}

void UKzInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregistering ends whatever was running on us, so nobody holds a dead target
	if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
	{
		Subsystem->UnregisterInteractable(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UKzInteractableComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	// Asked about every scan and every validation, so gathered once instead of on each question
	Handlers.Reset();
	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			Handlers.Add(OwnerActor);
		}
		for (UActorComponent* Component : OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass()))
		{
			if (Component != this)
			{
				Handlers.Add(Component);
			}
		}
	}

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (UKzInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzInteractionSubsystem>())
		{
			Subsystem->RegisterInteractable(this);
		}
	}
}

void UKzInteractableComponent::Deactivate()
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (UKzInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzInteractionSubsystem>())
		{
			Subsystem->UnregisterInteractable(this);
		}
	}

	Super::Deactivate();
}

bool UKzInteractableComponent::GetInteractionSpot(FKzTransformSource& OutSpot) const
{
	if (!bRequiresInteractionSpot)
	{
		return false;
	}

	FTransform Resolved;
	OutSpot = InteractionSpot.GetSocketTransform(this, Resolved) ? InteractionSpot.ToTransformSource(this) : FKzTransformSource(this);
	return true;
}

bool UKzInteractableComponent::CanInteract(UKzInteractorComponent* Interactor) const
{
	if (!Interactor) return false;

	if (IsInteractionFull() && !HasInteractor(Interactor))
	{
		return false;
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	KzInteraction::FillContext(InteractionRequirement, Interactor, MutableThis);

	if (!FScriptableRequirement::EvaluateRequirement(Interactor, InteractionRequirement))
	{
		return false;
	}

	if (!GetOwner()) return false;

	return ForEachHandler([Interactor, MutableThis](UObject* Handler) { return IKzInteractableInterface::Execute_CanInteract(Handler, Interactor, MutableThis); });
}

bool UKzInteractableComponent::GetAvailability(UKzInteractorComponent* Interactor, FGameplayTag& OutReason) const
{
	OutReason = FGameplayTag();

	if (!Interactor)
	{
		return false;
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	KzInteraction::FillContext(AvailabilityRequirement, Interactor, MutableThis);

	if (!FScriptableRequirement::EvaluateRequirement(Interactor, AvailabilityRequirement))
	{
		OutReason = UnavailableReason;
		return false;
	}

	if (!GetOwner())
	{
		return false;
	}

	return ForEachHandler([Interactor, MutableThis, &OutReason](UObject* Handler) { return IKzInteractableInterface::Execute_GetInteractionAvailability(Handler, Interactor, MutableThis, OutReason); });
}

const FKzInteractionAction* UKzInteractableComponent::FindAction(FGameplayTag InputTag) const
{
	if (!InputTag.IsValid())
	{
		return nullptr;
	}

	return Actions.FindByPredicate([InputTag](const FKzInteractionAction& Action) { return Action.InputTag == InputTag; });
}

FGameplayTagContainer UKzInteractableComponent::GetActionInputTags() const
{
	FGameplayTagContainer Tags;
	for (const FKzInteractionAction& Action : Actions)
	{
		Tags.AddTag(Action.InputTag);
	}

	return Tags;
}

bool UKzInteractableComponent::GetAction(FGameplayTag InputTag, FKzInteractionAction& OutAction) const
{
	if (const FKzInteractionAction* Action = FindAction(InputTag))
	{
		OutAction = *Action;
		return true;
	}

	return false;
}

bool UKzInteractableComponent::CanRunAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor) const
{
	const FKzInteractionAction* Action = FindAction(InputTag);
	if (!Action || !Interactor)
	{
		return false;
	}

	if (Action->Cooldown > 0.0f)
	{
		const double* LastUse = LastActionTimes.Find(FActionKey(Interactor, InputTag));
		const UWorld* World = GetWorld();

		if (LastUse && World && World->GetTimeSeconds() - *LastUse < Action->Cooldown)
		{
			return false;
		}
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	KzInteraction::FillContext(Action->Requirement, Interactor, MutableThis);

	return FScriptableRequirement::EvaluateRequirement(Interactor, Action->Requirement);
}

bool UKzInteractableComponent::RunAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor)
{
	// Re-checked here because the animation put time between the press and this call
	if (!CanRunAction(InputTag, Interactor))
	{
		return false;
	}

	FKzInteractionAction* Action = Actions.FindByPredicate([InputTag](const FKzInteractionAction& Candidate) { return Candidate.InputTag == InputTag; });
	if (!Action)
	{
		return false;
	}

	if (const UWorld* World = GetWorld())
	{
		// Instigators that are gone leave their times behind
		for (auto It = LastActionTimes.CreateIterator(); It; ++It)
		{
			if (!It.Key().Key.IsValid())
			{
				It.RemoveCurrent();
			}
		}
		LastActionTimes.Add(FActionKey(Interactor, InputTag), World->GetTimeSeconds());
	}

	KzInteraction::FillContext(Action->Effect, Interactor, this);
	Action->Effect.Run(this);

	// Then the handlers, for whatever the action means in code rather than in data
	ForEachHandler([this, Interactor, InputTag](UObject* Handler)
	{
		IKzInteractableInterface::Execute_OnInteractionAction(Handler, Interactor, this, InputTag);
		return true;
	});

	return true;
}

void UKzInteractableComponent::PressPerformedAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor)
{
	if (!PerformedAction.IsValid())
	{
		StartPerformedAction(InputTag, Interactor);
	}
	else if (bPerformedActionLanded)
	{
		KeptPress = InputTag;
		KeptPressInteractor = Interactor;
	}
}

void UKzInteractableComponent::StartPerformedAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor)
{
	if (!CanRunAction(InputTag, Interactor))
	{
		return;
	}

	PerformedAction = InputTag;
	PerformingInteractor = Interactor;
	bPerformedActionLanded = false;
	ForEachHandler([this, Interactor, InputTag](UObject* Handler)
	{
		IKzInteractableInterface::Execute_OnPerformedActionStarted(Handler, Interactor, this, InputTag);
		return true;
	});
	OnActionStarted.Broadcast(Interactor, InputTag);
}

void UKzInteractableComponent::LandAction()
{
	if (!PerformedAction.IsValid() || bPerformedActionLanded)
	{
		return;
	}

	bPerformedActionLanded = true;
	RunAction(PerformedAction, PerformingInteractor.Get());
}

void UKzInteractableComponent::FinishAction()
{
	if (!PerformedAction.IsValid())
	{
		return;
	}

	const FGameplayTag Next = KeptPress;
	UKzInteractorComponent* Interactor = KeptPressInteractor.Get();
	PerformedAction = FGameplayTag();
	PerformingInteractor.Reset();
	bPerformedActionLanded = false;
	KeptPress = FGameplayTag();
	KeptPressInteractor.Reset();

	// A press kept while it ran starts the next one right away, as long as whoever made it still interacts
	if (Next.IsValid() && Interactor && HasInteractor(Interactor))
	{
		StartPerformedAction(Next, Interactor);
	}
}

bool UKzInteractableComponent::ForEachHandler(TFunctionRef<bool(UObject*)> Visit) const
{
	for (const TWeakObjectPtr<UObject>& Handler : Handlers)
	{
		UObject* Object = Handler.Get();
		if (Object && !Visit(Object))
		{
			return false;
		}
	}
	return true;
}

int32 UKzInteractableComponent::GetInteractionCount() const
{
	const UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	return Subsystem ? Subsystem->CountInteractionsOn(this) : 0;
}

bool UKzInteractableComponent::IsInteractionFull() const
{
	if (MaxInteractors <= 0) return false; // 0 or less means unlimited

	return GetInteractionCount() >= MaxInteractors;
}

bool UKzInteractableComponent::HasInteractor(const UKzInteractorComponent* Interactor) const
{
	const UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	if (!Subsystem || !Interactor)
	{
		return false;
	}

	const FKzInteractionHandle Handle = Subsystem->FindInteractionFor(Interactor);
	const FKzInteraction* Interaction = Subsystem->FindInteraction(Handle);

	return Interaction && Interaction->Interactable.Get() == this;
}

bool UKzInteractableComponent::IsActorInteracting(const AActor* Actor) const
{
	const UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	return Subsystem && Subsystem->IsActorInteractingWith(Actor, this);
}

EKzInteractionResult UKzInteractableComponent::EvaluateInteractionResult(UKzInteractorComponent* Interactor)
{
	if (!Interactor) return EKzInteractionResult::Ignored;

	// Continuous > Completed > Ignored: a handler can raise the answer, never lower it
	EKzInteractionResult FinalResult = DefaultInteractionResult;
	ForEachHandler([this, Interactor, &FinalResult](UObject* Handler)
	{
		const EKzInteractionResult Result = IKzInteractableInterface::Execute_GetInteractionResult(Handler, Interactor, this);
		if (Result == EKzInteractionResult::Continuous) FinalResult = EKzInteractionResult::Continuous;
		else if (Result == EKzInteractionResult::Completed && FinalResult == EKzInteractionResult::Ignored) FinalResult = EKzInteractionResult::Completed;
		return true;
	});

	return FinalResult;
}

void UKzInteractableComponent::NotifyInteractionBegun(UKzInteractorComponent* Interactor, const FKzInteraction& Interaction)
{
	if (!Interactor) return;

	ForEachHandler([this, Interactor, &Interaction](UObject* Handler)
	{
		IKzInteractableInterface::Execute_OnInteractionBegun(Handler, Interactor, this, Interaction);
		return true;
	});

	OnInteract.Broadcast(Interactor);

	KzInteraction::FillContext(InteractionAction, Interactor, this);
	InteractionAction.Run(this);
}

void UKzInteractableComponent::NotifyInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason)
{
	// An action whose instigator leaves halfway lands nothing more, but still finishes, so a press kept from somebody else starts the next
	if (PerformedAction.IsValid() && PerformingInteractor == Interaction.Interactor)
	{
		PerformingInteractor.Reset();
		bPerformedActionLanded = true;
	}
	if (KeptPress.IsValid() && KeptPressInteractor == Interaction.Interactor)
	{
		KeptPress = FGameplayTag();
		KeptPressInteractor.Reset();
	}

	if (!GetOwner())
	{
		return;
	}

	ForEachHandler([&Interaction, Reason](UObject* Handler)
	{
		IKzInteractableInterface::Execute_OnInteractionEnded(Handler, Interaction, Reason);
		return true;
	});

	OnInteractionEnded.Broadcast(Interaction.Interactor.Get(), Reason);
}

bool UKzInteractableComponent::ShouldKeepInteractionAlive(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason) const
{
	// Range first, since it is the one rule every continuous interaction shares
	if (KeepAliveRange > 0.0f)
	{
		const AActor* Holder = Interaction.Instigator.Get();
		if (!Holder || FVector::Dist(Holder->GetActorLocation(), GetComponentLocation()) > KeepAliveRange)
		{
			OutReason = EKzInteractionEndReason::OutOfRange;
			return false;
		}
	}

	if (!GetOwner())
	{
		OutReason = EKzInteractionEndReason::TargetLost;
		return false;
	}

	// The same data that decided whether this could start, asked again for as long as it runs
	if (bEndIfUnavailable)
	{
		FGameplayTag UnusedReason;
		if (!GetAvailability(Interaction.Interactor.Get(), UnusedReason))
		{
			OutReason = EKzInteractionEndReason::ConditionFailed;
			return false;
		}
	}

	return ForEachHandler([&Interaction, &OutReason](UObject* Handler) { return IKzInteractableInterface::Execute_ShouldKeepInteractionAlive(Handler, Interaction, OutReason); });
}

void UKzInteractableComponent::StopAllInteractions(EKzInteractionEndReason Reason)
{
	if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
	{
		Subsystem->EndInteractionsOn(this, Reason);
	}
}