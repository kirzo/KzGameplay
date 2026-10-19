// Copyright 2026 kirzo

#include "Interaction/KzInteractableGroupComponent.h"
#include "Interaction/KzInteractorComponent.h"

UKzInteractableGroupComponent::UKzInteractableGroupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	KzInteraction::DeclareContext(InteractionAction);
	KzInteraction::DeclareContext(WhileAction);
	KzInteraction::DeclareEndContext(EndAction);
}

bool UKzInteractableGroupComponent::IsMember(const UKzInteractableComponent* Interactable) const
{
	// Only the owner's own interactables reach us, so an empty list already means them all
	return Members.IsEmpty() || (Interactable && Members.ContainsByPredicate([this, Interactable](const FKzComponentReference& Member) { return Member.GetComponent(this) == Interactable; }));
}

void UKzInteractableGroupComponent::OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction)
{
	if (IsMember(Interactable))
	{
		OnBegun.Broadcast(Interactable, Interactor);
	}
}

void UKzInteractableGroupComponent::OnInteractionEnded_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason Reason)
{
	if (IsMember(Interaction.Interactable.Get()))
	{
		OnEnded.Broadcast(Interaction.Interactable.Get(), Interaction.Interactor.Get(), Reason, Interaction.EndDetail);
	}
}

void UKzInteractableGroupComponent::OnInteractionAction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag)
{
	if (IsMember(Interactable))
	{
		OnActionLanded.Broadcast(Interactable, Interactor, InputTag);
	}
}

void UKzInteractableGroupComponent::OnPerformedActionStarted_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag)
{
	if (IsMember(Interactable))
	{
		OnActionStarted.Broadcast(Interactable, Interactor, InputTag);
	}
}