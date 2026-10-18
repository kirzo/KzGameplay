// Copyright 2026 kirzo

#include "Interaction/KzInteractionEventsComponent.h"
#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractorComponent.h"

UKzInteractionEventsComponent::UKzInteractionEventsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzInteractionEventsComponent::OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction)
{
	OnBegun.Broadcast(Interactable, Interactor);
}

void UKzInteractionEventsComponent::OnInteractionEnded_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason Reason)
{
	OnEnded.Broadcast(Interaction.Interactable.Get(), Interaction.Interactor.Get(), Reason);
}

void UKzInteractionEventsComponent::OnInteractionAction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag)
{
	OnActionLanded.Broadcast(Interactable, Interactor, InputTag);
}

void UKzInteractionEventsComponent::OnPerformedActionStarted_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag)
{
	OnActionStarted.Broadcast(Interactable, Interactor, InputTag);
}