// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "Interaction/KzInteractableComponent.h"
#include "Scoring/KzTargetScoringProfile.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "KzInteractorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentInteractableChangedDelegate, UKzInteractableComponent*, NewInteractable, UKzInteractableComponent*, OldInteractable);

/**
 * Component attached to the Player (or AI) responsible for finding and evaluating Interactables.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzInteractorComponent : public UKzShapeComponent
{
	GENERATED_BODY()

public:
	UKzInteractorComponent();

	// ==========================================
	// CONFIGURATION
	// ==========================================

	/** How often to scan the environment (in seconds). 0.1s is usually perfect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings", meta = (ClampMin = "0.01"))
	float ScanRate;

	/**
	 * Hard Filters: If the interactable doesn't pass these requirements, it is immediately discarded.
	 * (e.g., Line of Sight check, Require specific Gameplay Tags).
	 */
	UPROPERTY(EditAnywhere, Category = "Interaction|Evaluation")
	FScriptableRequirement FilterRequirement;

	/**
	 * Soft Scoring: Evaluators used to grade the valid candidates and pick the absolute best one.
	 * (e.g., Highest score based on Angle/DotProduct and Distance).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Evaluation")
	FKzTargetScoringProfile ScoringProfile;

	// ==========================================
	// DELEGATES
	// ==========================================

	/** Fired when the best candidate changes. Ideal for updating the UI Prompt. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnCurrentInteractableChangedDelegate OnCurrentInteractableChanged;

	// ==========================================
	// RUNTIME LOGIC
	// ==========================================

	/** Returns the current best interactable candidate. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	UKzInteractableComponent* GetCurrentInteractable() const { return CurrentInteractable.Get(); }

	/** * Call this from your PlayerController/Character when the 'Interact' input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool Interact();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The timer function that periodically scans the world. */
	void PerformScan();

	/** The currently focused best candidate. */
	TWeakObjectPtr<UKzInteractableComponent> CurrentInteractable;

	/** Timer handle for the scanning loop. */
	FTimerHandle ScanTimerHandle;

	// ==========================================
	// NETWORKING
	// ==========================================

	/**
	 * RPC sent to the server to request interaction with a specific target.
	 * The server should ideally validate the interaction before executing it.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TryInteract(UKzInteractableComponent* Target);
};