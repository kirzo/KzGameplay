// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/KzInteractableInterface.h"
#include "KzInteractionEventsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzInteractionEventsBegunDelegate, UKzInteractableComponent*, Interactable, UKzInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzInteractionEventsEndedDelegate, UKzInteractableComponent*, Interactable, UKzInteractorComponent*, Interactor, EKzInteractionEndReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzInteractionEventsActionDelegate, UKzInteractableComponent*, Interactable, UKzInteractorComponent*, Interactor, FGameplayTag, InputTag);

/**
 * Every interaction with the owner's interactables, from all of them at once and saying which one: the Blueprint side of the interactable
 * interface, for an object with several interactables, such as a table with one per side. A native handler, so it never vetoes anything.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent, DisplayName = "Interaction Events"))
class KZGAMEPLAY_API UKzInteractionEventsComponent : public UActorComponent, public IKzInteractableInterface
{
	GENERATED_BODY()

public:
	UKzInteractionEventsComponent();

	/** An interaction with one of the owner's interactables began. On every machine. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractionEventsBegunDelegate OnBegun;

	/** An interaction with one of the owner's interactables ended, whoever ended it. On every machine; the Interactor may already be gone. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractionEventsEndedDelegate OnEnded;

	/** An action the owner performs itself started, for it to play it and call Land Action and Finish Action on the Interactable. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractionEventsActionDelegate OnActionStarted;

	/** The effect of an action landed, whichever way the action is played. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FKzInteractionEventsActionDelegate OnActionLanded;

protected:
	virtual void OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction) override;
	virtual void OnInteractionEnded_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason Reason) override;
	virtual void OnInteractionAction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag) override;
	virtual void OnPerformedActionStarted_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag) override;
};