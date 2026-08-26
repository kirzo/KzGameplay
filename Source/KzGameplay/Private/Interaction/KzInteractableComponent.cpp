// Copyright 2026 kirzo

#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractableInterface.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UE_DISABLE_OPTIMIZATION

void KzInteraction::DeclareContext(FScriptableContainer& Container)
{
	Container.AddContextProperty<AActor*>(TEXT("Instigator"));
	Container.AddContextProperty<UKzInteractorComponent*>(TEXT("Interactor"));
	Container.AddContextProperty<UKzInteractableComponent*>(TEXT("Interactable"));
	Container.AddContextProperty<AActor*>(TEXT("Target"));
}

FKzInteractionAction::FKzInteractionAction()
{
	KzInteraction::DeclareContext(Requirement);
	KzInteraction::DeclareContext(Effect);
}

UKzInteractableComponent::UKzInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionTime = 0.0f;

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

bool UKzInteractableComponent::GetInteractionTransform(FTransform& OutTransform) const
{
	if (!bRequiresInteractionSpot)
	{
		return false;
	}

	if (InteractionSpot.GetSocketTransform(this, OutTransform))
	{
		return true;
	}

	OutTransform = GetComponentTransform();
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

	InteractionRequirement.ResetContext();
	InteractionRequirement.SetContextProperty(TEXT("Instigator"), Interactor->GetOwner());
	InteractionRequirement.SetContextProperty(TEXT("Interactor"), Interactor);
	InteractionRequirement.SetContextProperty(TEXT("Interactable"), MutableThis);

	if (!FScriptableRequirement::EvaluateRequirement(Interactor, InteractionRequirement))
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return false;

	if (OwnerActor->Implements<UKzInteractableInterface>())
	{
		if (!IKzInteractableInterface::Execute_CanInteract(OwnerActor, Interactor, MutableThis))
		{
			return false;
		}
	}

	TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
	for (UActorComponent* Comp : SiblingComponents)
	{
		if (Comp != this)
		{
			if (!IKzInteractableInterface::Execute_CanInteract(Comp, Interactor, MutableThis))
			{
				return false;
			}
		}
	}

	return true;
}

bool UKzInteractableComponent::GetAvailability(UKzInteractorComponent* Interactor, FGameplayTag& OutReason) const
{
	OutReason = FGameplayTag();

	if (!Interactor)
	{
		return false;
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	AvailabilityRequirement.ResetContext();
	AvailabilityRequirement.SetContextProperty(TEXT("Instigator"), Interactor->GetOwner());
	AvailabilityRequirement.SetContextProperty(TEXT("Interactor"), Interactor);
	AvailabilityRequirement.SetContextProperty(TEXT("Interactable"), MutableThis);

	if (!FScriptableRequirement::EvaluateRequirement(Interactor, AvailabilityRequirement))
	{
		OutReason = UnavailableReason;
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	if (OwnerActor->Implements<UKzInteractableInterface>())
	{
		if (!IKzInteractableInterface::Execute_GetInteractionAvailability(OwnerActor, Interactor, MutableThis, OutReason))
		{
			return false;
		}
	}

	TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
	for (UActorComponent* Component : SiblingComponents)
	{
		if (Component != MutableThis)
		{
			if (!IKzInteractableInterface::Execute_GetInteractionAvailability(Component, Interactor, MutableThis, OutReason))
			{
				return false;
			}
		}
	}

	return true;
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
		const double* LastUse = LastActionTime.Find(InputTag);
		const UWorld* World = GetWorld();

		if (LastUse && World && World->GetTimeSeconds() - *LastUse < Action->Cooldown)
		{
			return false;
		}
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	Action->Requirement.ResetContext();
	Action->Requirement.SetContextProperty(TEXT("Instigator"), Interactor->GetOwner());
	Action->Requirement.SetContextProperty(TEXT("Interactor"), Interactor);
	Action->Requirement.SetContextProperty(TEXT("Interactable"), MutableThis);
	Action->Requirement.SetContextProperty(TEXT("Target"), GetOwner());

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
		LastActionTime.Add(InputTag, World->GetTimeSeconds());
	}

	Action->Effect.SetContextProperty(TEXT("Instigator"), Interactor->GetOwner());
	Action->Effect.SetContextProperty(TEXT("Interactor"), Interactor);
	Action->Effect.SetContextProperty(TEXT("Interactable"), this);
	Action->Effect.SetContextProperty(TEXT("Target"), GetOwner());
	Action->Effect.Run(this);

	// Then the handlers, for whatever the action means in code rather than in data
	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			IKzInteractableInterface::Execute_OnInteractionAction(OwnerActor, Interactor, this, InputTag);
		}

		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				IKzInteractableInterface::Execute_OnInteractionAction(Component, Interactor, this, InputTag);
			}
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

	EKzInteractionResult FinalResult = DefaultInteractionResult;
	AActor* OwnerActor = GetOwner();

	if (OwnerActor)
	{
		auto Escalate = [&FinalResult](EKzInteractionResult Result)
			{
				// Continuous > Completed > Ignored: a handler can raise the answer, never lower it
				if (Result == EKzInteractionResult::Continuous) FinalResult = EKzInteractionResult::Continuous;
				else if (Result == EKzInteractionResult::Completed && FinalResult == EKzInteractionResult::Ignored) FinalResult = EKzInteractionResult::Completed;
			};

		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			Escalate(IKzInteractableInterface::Execute_GetInteractionResult(OwnerActor, Interactor, this));
		}

		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				Escalate(IKzInteractableInterface::Execute_GetInteractionResult(Component, Interactor, this));
			}
		}
	}

	return FinalResult;
}

void UKzInteractableComponent::NotifyInteractionBegun(UKzInteractorComponent* Interactor, const FKzInteraction& Interaction)
{
	if (!Interactor) return;

	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			IKzInteractableInterface::Execute_OnInteractionBegun(OwnerActor, Interactor, this, Interaction);
		}

		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				IKzInteractableInterface::Execute_OnInteractionBegun(Component, Interactor, this, Interaction);
			}
		}
	}

	OnInteract.Broadcast(Interactor);

	InteractionAction.SetContextProperty(TEXT("Instigator"), Interactor->GetOwner());
	InteractionAction.SetContextProperty(TEXT("Interactor"), Interactor);
	InteractionAction.SetContextProperty(TEXT("Interactable"), this);
	InteractionAction.SetContextProperty(TEXT("Target"), GetOwner());
	InteractionAction.Run(this);
}

void UKzInteractableComponent::NotifyInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (OwnerActor->Implements<UKzInteractableInterface>())
	{
		IKzInteractableInterface::Execute_OnInteractionEnded(OwnerActor, Interaction, Reason);
	}

	TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
	for (UActorComponent* Component : SiblingComponents)
	{
		if (Component != this)
		{
			IKzInteractableInterface::Execute_OnInteractionEnded(Component, Interaction, Reason);
		}
	}
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

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		OutReason = EKzInteractionEndReason::TargetLost;
		return false;
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	if (OwnerActor->Implements<UKzInteractableInterface>())
	{
		if (!IKzInteractableInterface::Execute_ShouldKeepInteractionAlive(OwnerActor, Interaction, OutReason))
		{
			return false;
		}
	}

	TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
	for (UActorComponent* Component : SiblingComponents)
	{
		if (Component != MutableThis)
		{
			if (!IKzInteractableInterface::Execute_ShouldKeepInteractionAlive(Component, Interaction, OutReason))
			{
				return false;
			}
		}
	}

	return true;
}

void UKzInteractableComponent::StopAllInteractions(EKzInteractionEndReason Reason)
{
	if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
	{
		Subsystem->EndInteractionsOn(this, Reason);
	}
}

UE_ENABLE_OPTIMIZATION