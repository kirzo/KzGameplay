// Copyright 2026 kirzo

#include "Interaction/InteractableComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractionSubsystem.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionTime = 0.0f;
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UInteractionSubsystem>())
	{
		Subsystem->UnregisterInteractable(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UInteractableComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (UInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UInteractionSubsystem>())
		{
			Subsystem->RegisterInteractable(this);
		}
	}
}

void UInteractableComponent::Deactivate()
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (UInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UInteractionSubsystem>())
		{
			Subsystem->UnregisterInteractable(this);
		}
	}

	Super::Deactivate();
}

FTransform UInteractableComponent::GetInteractionTransform() const
{
	FTransform OutTransform;
	if (InteractionSpot.GetSocketTransform(this, OutTransform))
	{
		return OutTransform;
	}

	// Fallback: return the center of this shape component
	return GetComponentTransform();
}

bool UInteractableComponent::ExecuteInteraction(AActor* Interactor)
{
	if (!Interactor) return false;

	// Broadcast the interaction to Blueprints or other listeners (like our ItemComponent)
	OnInteract.Broadcast(Interactor);

	return true;
}