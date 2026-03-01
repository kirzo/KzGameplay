// Copyright 2026 kirzo

#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractableInterface.h"

#include "GameFramework/Actor.h"

UKzInteractableComponent::UKzInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionTime = 0.0f;
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

FTransform UKzInteractableComponent::GetInteractionTransform() const
{
	FTransform OutTransform;
	if (InteractionSpot.GetSocketTransform(this, OutTransform))
	{
		return OutTransform;
	}

	// Fallback: return the center of this shape component
	return GetComponentTransform();
}

bool UKzInteractableComponent::ExecuteInteraction(UKzInteractorComponent* Interactor)
{
	if (!Interactor) return false;

	bool bWasHandled = false;

	// Broadcast the event for generic listeners (UI, Audio, Quest Trackers, etc.)
	OnInteract.Broadcast(Interactor);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		// Check if the Actor itself implements the interface (Great for simple Level Design Blueprints)
		if (OwnerActor->Implements<UKzInteractableInterface>())
		{
			bWasHandled |= IKzInteractableInterface::Execute_HandleInteraction(OwnerActor, Interactor, this);
		}

		// Find all sibling components that implement the interface and trigger them
		TArray<UActorComponent*> SiblingComponents = OwnerActor->GetComponentsByInterface(UKzInteractableInterface::StaticClass());
		for (UActorComponent* Component : SiblingComponents)
		{
			if (Component != this)
			{
				bWasHandled |= IKzInteractableInterface::Execute_HandleInteraction(Component, Interactor, this);
			}
		}
	}

	// We return true if at least one system (Actor or Component) handled the interaction logic
	return bWasHandled;
}