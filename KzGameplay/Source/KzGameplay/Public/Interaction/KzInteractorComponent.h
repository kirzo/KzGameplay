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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractorInteractionEndedDelegate, UKzInteractableComponent*, Interactable, EKzInteractionEndReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFocusAvailabilityChangedDelegate, UKzInteractableComponent*, Interactable, bool, bAvailable, FGameplayTag, Reason);

/**
 * What the server tells everyone about this interactor's engagement. Interaction handles are local to each
 * machine, so identity travels as the target plus a sequence number: a new sequence means a new interaction,
 * even with the same target.
 */
USTRUCT()
struct FKzReplicatedInteraction
{
	GENERATED_BODY()

	/** What we are engaged with, or null when free. */
	UPROPERTY()
	TObjectPtr<UKzInteractableComponent> Interactable = nullptr;

	/** Bumped on every change, so back-to-back interactions with the same target still register. */
	UPROPERTY()
	uint8 Sequence = 0;

	/** Why the last one ended, so clients can react the same way the server did. */
	UPROPERTY()
	EKzInteractionEndReason LastEndReason = EKzInteractionEndReason::Released;
};

#if WITH_GAMEPLAY_DEBUGGER
struct FKzInteractionDebugCandidate
{
	TWeakObjectPtr<class UKzInteractableComponent> Interactable;

	float Score;
	bool bPassedFilters;
	bool bIsBest;
};
#endif

/**
 * Component attached to the Player (or AI) responsible for finding and evaluating Interactables.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzInteractorComponent : public UKzShapeComponent
{
	GENERATED_BODY()

public:
	UKzInteractorComponent();

#if WITH_GAMEPLAY_DEBUGGER
	/** Caches the results of the last scan for the Gameplay Debugger. */
	TArray<FKzInteractionDebugCandidate> LastDebugCandidates;
#endif

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

	/**
	 * Gameplay event sent to the owner when an interaction of ours begins, carrying the interactable as
	 * OptionalObject. An ability that should run for the duration triggers on this rather than on input.
	 * Leave empty to send nothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings", meta = (Categories = "Interaction"))
	FGameplayTag InteractionBegunEventTag;

	/**
	 * Gameplay event sent to the owner whenever an interaction of ours ends, whatever ended it.
	 * List it in an ability's AbilityCancelTriggers to have that ability cancel with the interaction.
	 * Leave empty to send nothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings", meta = (Categories = "Interaction"))
	FGameplayTag InteractionEndedEventTag;

	// ==========================================
	// DELEGATES
	// ==========================================

	/** Fired when the best candidate changes. Ideal for updating the UI Prompt. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnCurrentInteractableChangedDelegate OnCurrentInteractableChanged;

	/**
	 * Fired when our interaction ends, whoever ended it: us, the object, or the world.
	 * An ability driving the interaction should listen here rather than assume it is the only one who can end it.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractorInteractionEndedDelegate OnInteractionEnded;

	/**
	 * Fired when the focused interactable becomes usable or stops being usable, with the reason.
	 * This is what a prompt listens to in order to grey itself out and say why.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnFocusAvailabilityChangedDelegate OnFocusAvailabilityChanged;

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

	/**
	 * Starts an interaction with the current focus.
	 * On a client without authority this asks the server and returns Pending: the real outcome arrives
	 * through OnInteractionEnded / the subsystem's OnInteractionBegun, never as a return value.
	 */
	/** Ends the current continuous interaction, if any, as a deliberate release. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopCurrentInteraction();

	/** Ends the current continuous interaction, if any, stating why. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndCurrentInteraction(EKzInteractionEndReason Reason);

	/** Returns true if we are currently locked into a continuous interaction. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractingContinuously() const { return CurrentInteraction.IsValid(); }

	/** The interactable we are engaged with, or null if we are free. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	UKzInteractableComponent* GetActiveInteractable() const;

	/** Whether the focused interactable can be used right now. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsFocusAvailable() const { return bFocusAvailable; }

	/** Why the focused interactable cannot be used, if it cannot. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FGameplayTag GetFocusUnavailableReason() const { return FocusUnavailableReason; }

	/** Handle of our live interaction, for code that needs to talk to the subsystem about it. */
	FKzInteractionHandle GetCurrentInteraction() const { return CurrentInteraction; }

	/**
	 * Records an interaction of ours that just began and announces it.
	 * Called by the subsystem before anything else is told, so handlers that ask what we are interacting
	 * with already have an answer.
	 */
	void HandleInteractionBegun(const FKzInteraction& Interaction);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Starts or resumes the environment scanning timer. */
	void StartScanning();

	/** Pauses the environment scanning timer. Useful when locked in a continuous interaction. */
	void StopScanning();

	/** The timer function that periodically scans the world. */
	void PerformScan();

	/** The currently focused best candidate. */
	TWeakObjectPtr<UKzInteractableComponent> CurrentFocus;

	/** Cached availability of the focus, so the delegate only fires on change. */
	bool bFocusAvailable = true;
	FGameplayTag FocusUnavailableReason;

	/** Re-evaluates the focus availability and reports it if it changed. */
	void UpdateFocusAvailability();

	/** Our live interaction, owned by the subsystem. We keep the handle, never the state. */
	FKzInteractionHandle CurrentInteraction;

	/** The server's view of what we are engaged with. Drives clients into the same interaction we have. */
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedInteraction)
	FKzReplicatedInteraction ReplicatedInteraction;

	/** Opens or closes the local interaction so it matches what the server just told us. */
	UFUNCTION()
	void OnRep_ReplicatedInteraction(const FKzReplicatedInteraction& OldValue);

	/** Asks the authority to start an interaction on our behalf. */
	UFUNCTION(Server, Reliable)
	void ServerInteractWith(UKzInteractableComponent* Target);

	/** Asks the authority to end our interaction. */
	UFUNCTION(Server, Reliable)
	void ServerEndInteraction(EKzInteractionEndReason Reason);

	/** Publishes our current engagement to clients. Authority only. */
	void UpdateReplicatedInteraction(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason);

	/** Mirrors an interaction the server started, without re-deciding whether it was allowed. */
	void MirrorServerInteraction(UKzInteractableComponent* Target);

	/** True where interactions are decided rather than mirrored. */
	bool HasInteractionAuthority() const;

	/** Routes the subsystem's end-of-interaction notification to our own listeners. */
	UFUNCTION()
	void HandleInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason);

	/** Timer handle for the scanning loop. */
	FTimerHandle ScanTimerHandle;
};