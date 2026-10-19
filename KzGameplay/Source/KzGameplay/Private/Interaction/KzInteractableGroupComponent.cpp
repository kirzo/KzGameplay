// Copyright 2026 kirzo

#include "Interaction/KzInteractableGroupComponent.h"
#include "Interaction/KzInteractorComponent.h"

UKzInteractableGroupComponent::UKzInteractableGroupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	KzInteraction::DeclareContext(InteractionRequirement);
}

bool UKzInteractableGroupComponent::IsMember(const UKzInteractableComponent* Interactable) const
{
	// Only the owner's own interactables ask their siblings, so an empty list already means them all
	return Members.IsEmpty() || (Interactable && Members.ContainsByPredicate([this, Interactable](const FKzComponentReference& Member) { return Member.GetComponent(this) == Interactable; }));
}

bool UKzInteractableGroupComponent::CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	if (!Interactable || !IsMember(Interactable))
	{
		return true;
	}

	KzInteraction::FillContext(InteractionRequirement, Interactor, Interactable);
	return FScriptableRequirement::EvaluateRequirement(Interactor, InteractionRequirement);
}

bool UKzInteractableGroupComponent::GetInteractionAvailability_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag& OutReason)
{
	if (!Interactable || !IsMember(Interactable))
	{
		return true;
	}

	if (const FKzAvailabilityRule* Failed = KzInteraction::FindFailedRule(AvailabilityRules, Interactor, Interactable, false))
	{
		OutReason = Failed->Reason;
		return false;
	}

	return true;
}

bool UKzInteractableGroupComponent::ShouldKeepInteractionAlive_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason)
{
	UKzInteractorComponent* Interactor = Interaction.Interactor.Get();
	UKzInteractableComponent* Interactable = Interaction.Interactable.Get();
	if (!Interactor || !Interactable || !IsMember(Interactable))
	{
		return true;
	}

	if (KzInteraction::FindFailedRule(AvailabilityRules, Interactor, Interactable, true))
	{
		OutReason = EKzInteractionEndReason::ConditionFailed;
		return false;
	}

	return true;
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
		OnEnded.Broadcast(Interaction.Interactable.Get(), Interaction.Interactor.Get(), Reason);
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