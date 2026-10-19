// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/KzComponentReference.h"
#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractableInterface.h"
#include "KzInteractableGroupComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzInteractableGroupBegunDelegate, UKzInteractableComponent*, Interactable, UKzInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzInteractableGroupEndedDelegate, UKzInteractableComponent*, Interactable, UKzInteractorComponent*, Interactor, EKzInteractionEndReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzInteractableGroupActionDelegate, UKzInteractableComponent*, Interactable, UKzInteractorComponent*, Interactor, FGameplayTag, InputTag);

/**
 * Several interactables of one actor that behave as one, such as the handles of a table: the rules they share, configured once
 * on top of each member's own, and the events of all of them in one place. An actor can have several groups.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent, DisplayName = "Interactable Group"))
class KZGAMEPLAY_API UKzInteractableGroupComponent : public UActorComponent, public IKzInteractableInterface
{
	GENERATED_BODY()

public:
	UKzInteractableGroupComponent();

	/** The owner's interactables in this group. Empty means all of them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowedClasses = "/Script/KzGameplay.KzInteractableComponent"))
	TArray<FKzComponentReference> Members;

	/** Hard requirement of every member, after its own: failing it removes the member from the scan, with no prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	mutable FScriptableRequirement InteractionRequirement;

	/** Soft requirements of every member, checked in order after its own; the first that fails names the reason. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (TitleProperty = "Reason"))
	mutable TArray<FKzAvailabilityRule> AvailabilityRules;

	/** An interaction with a member began. On every machine. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractableGroupBegunDelegate OnBegun;

	/** An interaction with a member ended, whoever ended it. On every machine; the Interactor may already be gone. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractableGroupEndedDelegate OnEnded;

	/** An action the owner performs itself started, for it to play it and call Land Action and Finish Action on the Interactable. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractableGroupActionDelegate OnActionStarted;

	/** The effect of an action landed, whichever way the action is played. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractableGroupActionDelegate OnActionLanded;

	/** Whether Interactable belongs to this group. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsMember(const UKzInteractableComponent* Interactable) const;

protected:
	virtual bool CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) override;
	virtual bool GetInteractionAvailability_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag& OutReason) override;
	virtual bool ShouldKeepInteractionAlive_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason) override;

	virtual void OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction) override;
	virtual void OnInteractionEnded_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason Reason) override;
	virtual void OnInteractionAction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag) override;
	virtual void OnPerformedActionStarted_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag) override;
};