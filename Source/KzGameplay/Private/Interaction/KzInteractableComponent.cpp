// Copyright 2026 kirzo

#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractableInterface.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UKzInteractableComponent::UKzInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionTime = 0.0f;

	InteractionRequirement.AddContextProperty<AActor*>(TEXT("Instigator"));
	InteractionRequirement.AddContextProperty<UKzInteractorComponent*>(TEXT("Interactor"));
	InteractionRequirement.AddContextProperty<UKzInteractableComponent*>(TEXT("Interactable"));

	InteractionAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	InteractionAction.AddContextProperty<UKzInteractorComponent*>(TEXT("Interactor"));
	InteractionAction.AddContextProperty<UKzInteractableComponent*>(TEXT("Interactable"));
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

EKzInteractionResult UKzInteractableComponent::ExecuteInteraction(UKzInteractorComponent* Interactor, const FKzInteraction& Interaction)
{
	if (!Interactor) return EKzInteractionResult::Ignored;

	EKzInteractionResult FinalResult = DefaultInteractionResult;
	AActor* OwnerActor = GetOwner();

	if (OwnerActor)
	{
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			EKzInteractionResult Result = IKzInteractableInterface::Execute_HandleInteraction(OwnerActor, Interactor, this, Interaction);

			// Escalate priority: Continuous > Completed > Ignored
			if (Result == EKzInteractionResult::Continuous) FinalResult = EKzInteractionResult::Continuous;
			else if (Result == EKzInteractionResult::Completed && FinalResult == EKzInteractionResult::Ignored) FinalResult = EKzInteractionResult::Completed;
		}

		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				EKzInteractionResult Result = IKzInteractableInterface::Execute_HandleInteraction(Component, Interactor, this, Interaction);

				if (Result == EKzInteractionResult::Continuous) FinalResult = EKzInteractionResult::Continuous;
				else if (Result == EKzInteractionResult::Completed && FinalResult == EKzInteractionResult::Ignored) FinalResult = EKzInteractionResult::Completed;
			}
		}
	}

	// Only broadcast if the interaction actually did something
	if (FinalResult != EKzInteractionResult::Ignored)
	{
		OnInteract.Broadcast(Interactor);

		InteractionAction.SetContextProperty(TEXT("Instigator"), Interactor->GetOwner());
		InteractionAction.SetContextProperty(TEXT("Interactor"), Interactor);
		InteractionAction.SetContextProperty(TEXT("Interactable"), this);
		InteractionAction.Run(this);
	}

	return FinalResult;
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
