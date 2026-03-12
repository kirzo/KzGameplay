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
	if (UKzInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzInteractionSubsystem>())
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
	// 1. Check if the designer explicitly disabled the need for a spot
	if (!bRequiresInteractionSpot)
	{
		return false;
	}

	// 2. Try to resolve the socket reference
	if (InteractionSpot.GetSocketTransform(this, OutTransform))
	{
		return true;
	}

	// 3. Fallback: If it requires a spot but the socket is invalid/missing, 
	// we default to the component's transform and warn the user.
	OutTransform = GetComponentTransform();

	UE_LOG(LogTemp, Warning, TEXT("Interactable [%s] requires an Interaction Spot, but the socket reference is invalid. Falling back to component transform."), *GetNameSafe(GetOwner()));

	return true;
}

bool UKzInteractableComponent::CanInteract(UKzInteractorComponent* Interactor) const
{
	if (!Interactor) return false;

	// Check capacity first. If we are full and this interactor is not already in the list, block it.
	if (IsInteractionFull() && !CurrentInteractors.Contains(Interactor))
	{
		return false;
	}

	UKzInteractableComponent* MutableThis = const_cast<UKzInteractableComponent*>(this);

	// 1. Evaluate the baseline Scriptable Requirement
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

	// 2. Query the Actor if it implements the interface
	if (OwnerActor->Implements<UKzInteractableInterface>())
	{
		if (!IKzInteractableInterface::Execute_CanInteract(OwnerActor, Interactor, MutableThis))
		{
			return false;
		}
	}

	// 3. Query all sibling components that implement the interface
	for (UActorComponent* Comp : OwnerActor->GetComponents())
	{
		if (Comp && Comp->Implements<UKzInteractableInterface>())
		{
			if (!IKzInteractableInterface::Execute_CanInteract(Comp, Interactor, MutableThis))
			{
				return false;
			}
		}
	}

	return true;
}

bool UKzInteractableComponent::IsInteractionFull() const
{
	if (MaxInteractors <= 0) return false; // 0 or less means unlimited

	// Count only valid interactors to prevent ghost blocks if an interactor was destroyed abruptly
	int32 ValidCount = 0;
	for (UKzInteractorComponent* Interactor : CurrentInteractors)
	{
		if (IsValid(Interactor))
		{
			ValidCount++;
		}
	}

	return ValidCount >= MaxInteractors;
}

bool UKzInteractableComponent::HasInteractor(const UKzInteractorComponent* Interactor) const
{
	return Interactor != nullptr && CurrentInteractors.Contains(Interactor);
}

bool UKzInteractableComponent::IsActorInteracting(const AActor* Actor) const
{
	if (!Actor) return false;

	for (UKzInteractorComponent* Interactor : CurrentInteractors)
	{
		if (IsValid(Interactor) && Interactor->GetOwner() == Actor)
		{
			return true;
		}
	}

	return false;
}

EKzInteractionResult UKzInteractableComponent::ExecuteInteraction(UKzInteractorComponent* Interactor)
{
	// Clean up any invalid pointers first
	CurrentInteractors.Remove(nullptr);

	if (!Interactor) return EKzInteractionResult::Ignored;

	// Safety check in case Execute is called without checking CanInteract first
	if (IsInteractionFull() && !CurrentInteractors.Contains(Interactor))
	{
		return EKzInteractionResult::Ignored;
	}

	EKzInteractionResult FinalResult = DefaultInteractionResult;
	AActor* OwnerActor = GetOwner();

	if (OwnerActor)
	{
		// 1. Check if the Actor itself implements the interface
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			EKzInteractionResult Result = IKzInteractableInterface::Execute_HandleInteraction(OwnerActor, Interactor, this);

			// Escalate priority: Continuous > Completed > Ignored
			if (Result == EKzInteractionResult::Continuous) FinalResult = EKzInteractionResult::Continuous;
			else if (Result == EKzInteractionResult::Completed && FinalResult == EKzInteractionResult::Ignored) FinalResult = EKzInteractionResult::Completed;
		}

		// 2. Find all sibling components that implement the interface
		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				EKzInteractionResult Result = IKzInteractableInterface::Execute_HandleInteraction(Component, Interactor, this);

				if (Result == EKzInteractionResult::Continuous) FinalResult = EKzInteractionResult::Continuous;
				else if (Result == EKzInteractionResult::Completed && FinalResult == EKzInteractionResult::Ignored) FinalResult = EKzInteractionResult::Completed;
			}
		}
	}

	// 3. Track continuous interactions
	if (FinalResult == EKzInteractionResult::Continuous)
	{
		CurrentInteractors.AddUnique(Interactor);
	}

	// 4. Only broadcast the generic event if the interaction actually did something
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

void UKzInteractableComponent::StopInteraction(UKzInteractorComponent* Interactor)
{
	if (!Interactor) return;

	CurrentInteractors.Remove(Interactor);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			IKzInteractableInterface::Execute_StopInteraction(OwnerActor, Interactor, this);
		}

		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				IKzInteractableInterface::Execute_StopInteraction(Component, Interactor, this);
			}
		}
	}
}