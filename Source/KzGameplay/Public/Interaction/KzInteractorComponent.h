// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "Interaction/KzInteractionTypes.h"
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
	UKzInteractableComponent* GetCurrentFocus() const { return CurrentFocus.Get(); }

	/** * Call this from your PlayerController/Character when the 'Interact' input is pressed. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	EKzInteractionResult Interact();

	/** Executes an interaction explicitly on a specific target, ignoring current focus. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	EKzInteractionResult InteractWith(UKzInteractableComponent* Target);

	/** Pauses the scanner without clearing the current focus (useful while walking towards a target). */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void PauseScanning();

	/** Resumes the scanner. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ResumeScanning();

	/** Manually aborts the current continuous interaction, if any. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopCurrentInteraction();

	/** Returns true if we are currently locked into a continuous interaction. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractingContinuously() const { return ActiveInteractable != nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Starts or resumes the environment scanning timer. */
	void StartScanning();

	/** Pauses the environment scanning timer. Useful when locked in a continuous interaction. */
	void StopScanning();

	/** The timer function that periodically scans the world. */
	void PerformScan();

	/** The currently focused best candidate. */
	TWeakObjectPtr<UKzInteractableComponent> CurrentFocus;

	/** The interactable we are currently engaged with in a continuous manner. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UKzInteractableComponent> ActiveInteractable;

	/** Timer handle for the scanning loop. */
	FTimerHandle ScanTimerHandle;

	// ==========================================
	// NETWORKING
	// ==========================================

	/**
	 * RPC sent to the server to request interaction with a specific target.
	 * The server should ideally validate the interaction before executing it.
	 */
	UFUNCTION(Server, Reliable)
	void Server_TryInteract(UKzInteractableComponent* Target);

	/** RPC sent to the server to manually abort the current continuous interaction. */
	UFUNCTION(Server, Reliable)
	void Server_StopCurrentInteraction();
};