// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "Components/KzComponentSocketReference.h"
#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFocusDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableActionDelegate, AActor*, Interactor);

/**
 * Represents an entity in the world that can be interacted with.
 */
UCLASS(Blueprintable, ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UInteractableComponent : public UKzShapeComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	// ==========================================
	// CONFIGURATION
	// ==========================================

	/** The action text to display in the UI (e.g., "Open Chest", "Pick Up Axe"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptText;

	/** How long the interaction button must be held. 0.0 means instant interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionTime;

	/**
	 * Optional specific point in space where the interaction should physically occur.
	 * Useful for AI pathfinding or Motion Warping (e.g., walking to the exact handle of a door).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FKzComponentSocketReference InteractionSpot;

	/**
	 * If true, the Interactor will trigger the interaction event automatically
	 * as soon as this object becomes the Best Candidate, without waiting for input.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Automatic")
	bool bIsAutomaticInteraction = false;

	/**
	 * If true, interaction triggers repeatedly while focused (useful for "zones").
	 * If false, it triggers once per focus session (useful for "enter state").
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Automatic", meta = (EditCondition = "bIsAutomaticInteraction"))
	bool bTriggerRepeatedly = false;

	// ==========================================
	// DELEGATES
	// ==========================================

	/** Fired when an interactor starts looking at this component (Local/Client only). */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableFocusDelegate OnBeginFocus;

	/** Fired when an interactor stops looking at this component (Local/Client only). */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableFocusDelegate OnEndFocus;

	/** Fired when the interaction has been successfully triggered and validated by the Server. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableActionDelegate OnInteract;

	// ==========================================
	// RUNTIME LOGIC
	// ==========================================

	/**
	 * Returns the exact world transform of the interaction spot.
	 * If no spot is defined or valid, it falls back to the component's center transform.
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FTransform GetInteractionTransform() const;

	/**
	 * Called to execute the interaction.
	 * @return True if the interaction was successfully handled.
	 */
	virtual bool ExecuteInteraction(AActor* Interactor);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};