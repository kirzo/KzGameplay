// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "Components/KzComponentReference.h"
#include "Interaction/KzInteractionTypes.h"
#include "Interaction/KzInteractionAction.h"
#include "ScriptableConditions/ScriptableRequirement.h" 
#include "ScriptableTasks/ScriptableAction.h" 
#include "KzInteractableComponent.generated.h"

class UKzInteractorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFocusDelegate, UKzInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableActionDelegate, UKzInteractorComponent*, Interactor);

/**
 * Represents an entity in the world that can be interacted with.
 */
UCLASS(Blueprintable, ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzInteractableComponent : public UKzShapeComponent
{
	GENERATED_BODY()

public:
	UKzInteractableComponent();

	// ==========================================
	// CONFIGURATION
	// ==========================================

	/** The action text to display in the UI (e.g., "Open Chest", "Pick Up Axe"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptText;

	/** How long the interaction button must be held. 0.0 means instant interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0"))
	float InteractionTime;

	/**
	 * The maximum number of simultaneous interactors allowed.
	 * 1 = Single user (Default). 0 or less = Unlimited interactors.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0"))
	int32 MaxInteractors = 1;

	/**
	 * How far an interactor may get from this component before a continuous interaction breaks by itself.
	 * 0 means no limit. Every continuous interaction wants this, so it lives here instead of in each actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0"))
	float KeepAliveRange = 0.0f;

	/**
	 * If true, this interactable requires the interactor to be at a specific spot.
	 * Useful for AI pathfinding or Motion Warping (e.g., walking to the exact handle of a door).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bRequiresInteractionSpot = false;

	/** The specific point in space where the interaction should physically occur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bRequiresInteractionSpot", EditConditionHides))
	FKzComponentSocketReference InteractionSpot;

	/**
	 * If true, this interactable can move around the world (e.g., a dropped item, a moving cart, an NPC).
	 * Keep this false for static objects (doors, chests, plants) to save CPU time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bIsDynamicInteraction = false;

	/**
	 * The default result returned if no components or the owner actor implements UKzInteractableInterface.
	 * Set to 'Completed' to allow this component to trigger its InteractionAction autonomously.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EKzInteractionResult DefaultInteractionResult = EKzInteractionResult::Ignored;

	/**
	 * Hard requirement: failing it removes this from the scan entirely, so there is no prompt and no UI.
	 * Use it for what the player should not even see, like a capability they do not have.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	mutable FScriptableRequirement InteractionRequirement;

	/**
	 * Soft requirement: failing it keeps this visible and focusable but blocks the interaction, reporting
	 * UnavailableReason. Use it for what the player should see and understand, like a missing key.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	mutable FScriptableRequirement AvailabilityRequirement;

	/** Reported when AvailabilityRequirement fails, for the UI to turn into words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTag UnavailableReason;

	/**
	 * Whether AvailabilityRequirement has to keep holding for the whole interaction, ending it if it stops.
	 * For rules that describe the object rather than the moment: a table that catches fire is no longer
	 * pushable, whether or not somebody had already grabbed it.
	 *
	 * Off by default because a start rule can become false by the very act of starting, and that would
	 * cancel the interaction it just allowed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bEndIfUnavailable = false;

	/** Fired when the interaction has been successfully triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FScriptableAction InteractionAction;

	/**
	 * What the instigator can do repeatedly while the interaction runs, keyed by the input that fires it.
	 * Lives here rather than in a bespoke component so anything interactable can offer actions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Actions", meta = (TitleProperty = "InputTag"))
	TArray<FKzInteractionAction> Actions;

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

	/** Fired when the interaction has been successfully triggered. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableActionDelegate OnInteract;

public:
	// ==========================================
	// RUNTIME LOGIC
	// ==========================================

	/**
	 * Returns the exact world transform of the interaction spot, if one is required and valid.
	 * @param OutTransform The transform of the interaction spot.
	 * @return True if a specific interaction spot is required and was successfully resolved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool GetInteractionTransform(FTransform& OutTransform) const;

	/** Evaluates if the given interactor can interact with this component. */
	virtual bool CanInteract(class UKzInteractorComponent* Interactor) const;

	/**
	 * Evaluates whether the interaction could run right now, and why not.
	 * Cheap enough to poll for the focused interactable, which is what feeds the prompt.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool GetAvailability(UKzInteractorComponent* Interactor, FGameplayTag& OutReason) const;

	/** The action fired by an input, or null if this interactable offers none for it. */
	const FKzInteractionAction* FindAction(FGameplayTag InputTag) const;

	/** Every input this interactable's actions listen to. */
	UFUNCTION(BlueprintPure, Category = "Interaction|Actions")
	FGameplayTagContainer GetActionInputTags() const;

	/** Copies out the action fired by an input, for Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Actions", meta = (DisplayName = "Find Action"))
	bool GetAction(FGameplayTag InputTag, FKzInteractionAction& OutAction) const;

	/** Whether an action can run right now: its own rules and its cooldown. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Actions")
	bool CanRunAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor) const;

	/**
	 * Runs an action's effect and starts its cooldown. Call it when the animation notify lands, or right
	 * away for an action with no animation. Re-checks the rules, so the wait cannot let a stale press through.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Actions")
	bool RunAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor);

	/** Returns true if the interactable has reached its maximum allowed interactors. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionFull() const;

	/** Returns the number of interactions currently running on this component. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	int32 GetInteractionCount() const;

	/** Returns true if the specified Interactor Component is currently interacting with this object. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasInteractor(const UKzInteractorComponent* Interactor) const;

	/** Returns true if the specified Actor is currently interacting with this object. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsActorInteracting(const AActor* Actor) const;

	/**
	 * Asks the owner and sibling handlers what kind of interaction this would be, without running any of
	 * it. Called by the subsystem before the interaction exists: deciding and acting are separate passes,
	 * so no handler can act on an answer that the others have not agreed to yet.
	 */
	virtual EKzInteractionResult EvaluateInteractionResult(UKzInteractorComponent* Interactor);

	/** Tells the owner and sibling handlers that the interaction is live. Called by the subsystem. */
	virtual void NotifyInteractionBegun(UKzInteractorComponent* Interactor, const FKzInteraction& Interaction);

	/** Tells the owner and sibling handlers that an interaction ended. Called by the subsystem. */
	virtual void NotifyInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason);

	/** Asks the range rule and the handlers whether a running interaction still holds. Called by the subsystem. */
	virtual bool ShouldKeepInteractionAlive(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason) const;

	/** Ends every interaction running on this component. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopAllInteractions(EKzInteractionEndReason Reason = EKzInteractionEndReason::Interrupted);

private:
	/**
	 * When each action last ran, for cooldowns.
	 * ponytail: keyed by action alone, which is enough while an interactable is used by one instigator at a
	 * time. Key it by instigator too if something ever offers shared actions to several at once.
	 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, double> LastActionTime;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
};