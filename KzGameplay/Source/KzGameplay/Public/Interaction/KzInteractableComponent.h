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
class UKzInteractableGroupComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFocusDelegate, UKzInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableActionDelegate, UKzInteractorComponent*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInteractableEndedDelegate, UKzInteractorComponent*, Interactor, EKzInteractionEndReason, Reason, FGameplayTag, Detail);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractableActionStartedDelegate, UKzInteractorComponent*, Interactor, FGameplayTag, InputTag);

/** What failing an interaction rule does before the interaction starts. */
UENUM(BlueprintType)
enum class EKzInteractionRuleFail : uint8
{
	/** Removed from the scan: no focus, no prompt, and the UI never hears of it. For what the player should not even see. */
	Hide,

	/** Focusable but blocked, reporting the rule's Reason. For what the player should see and understand, like full hands. */
	Block
};

/** A rule an interactable must meet, what failing it does, and the reason reported. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzInteractionRule
{
	GENERATED_BODY()

	FKzInteractionRule();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EKzInteractionRuleFail OnFail = EKzInteractionRuleFail::Hide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FScriptableRequirement Requirement;

	/** Shown to the UI when a Block rule fails, and the EndDetail of an interaction any rule ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTag Reason;

	/**
	 * Whether this rule has to keep holding for the whole interaction, ending it if it stops. For rules that describe the object rather
	 * than the moment: a table that catches fire is no longer pushable, whether or not somebody had already grabbed it.
	 *
	 * Off by default because a start rule can become false by the very act of starting, and that would cancel the interaction it just allowed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bEndIfFails = false;
};

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
	 * What this must meet to be used, checked in order, before those of the Interactable Groups it belongs to: any failing Hide rule
	 * removes it from the scan, the first failing Block rule names the reason the UI shows, and the first failing rule with bEndIfFails
	 * ends a running interaction with its Reason as the detail.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (TitleProperty = "{OnFail}: {Reason}"))
	mutable TArray<FKzInteractionRule> Rules;

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

	/** Fired when the interaction has been successfully triggered, on every machine, right after OnInteractionBegun. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableActionDelegate OnInteract;

	/**
	 * Fired when an interaction with this ends, whoever ended it, on every machine, right after OnInteractionEnded. The Interactor may already be gone.
	 * Detail says why beyond the reason when someone knew, such as the Reason of the rule that ended it.
	 * With OnInteract, it lets a Blueprint follow an interaction without implementing the interactable interface.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableEndedDelegate OnInteractionEnded;

	/** Fired when an action the object performs itself starts, for it to play it, as a lever its timeline, calling Land Action and Finish Action as it goes. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractableActionStartedDelegate OnActionStarted;

public:
	// ==========================================
	// RUNTIME LOGIC
	// ==========================================

	/**
	 * The interaction spot as a source that follows the object, so whoever walks to it arrives where it is now rather than where it was.
	 * Falls back to this component when the spot reference does not resolve.
	 * @return True if this interactable requires an interaction spot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool GetInteractionSpot(FKzTransformSource& OutSpot) const;

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

	/**
	 * A press of an action the object performs itself: starts it while none is running, keeps one press once the running one has landed
	 * its effect, to start the next as soon as it finishes, and ignores presses before that.
	 */
	void PressPerformedAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor);

	/** Lands the effect of the action the object is performing, at the moment of its own animation it is due. Once per action. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Actions")
	void LandAction();

	/** Ends the action the object is performing; a press kept meanwhile starts the next one at once. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Actions")
	void FinishAction();

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

	/** Asks the range, the rules and the handlers whether a running interaction still holds, with the detail of what broke it. Called by the subsystem. */
	virtual bool ShouldKeepInteractionAlive(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason, FGameplayTag& OutDetail) const;

	/** Ends every interaction running on this component. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopAllInteractions(EKzInteractionEndReason Reason = EKzInteractionEndReason::Interrupted);

private:
	/** When each instigator last ran each action, for cooldowns: several at once each keep their own pace. */
	TMap<TPair<TWeakObjectPtr<UKzInteractorComponent>, FGameplayTag>, double> LastActionTimes;

	/**
	 * The action the object is performing, one at a time since it is the object's own animation, and who pressed it; whether it can still
	 * land its effect; and a press kept for when it finishes, from whoever made it.
	 */
	FGameplayTag PerformedAction;
	TWeakObjectPtr<UKzInteractorComponent> PerformingInteractor;
	bool bPerformedActionLanded = false;
	FGameplayTag KeptPress;
	TWeakObjectPtr<UKzInteractorComponent> KeptPressInteractor;

	void StartPerformedAction(FGameplayTag InputTag, UKzInteractorComponent* Interactor);

	/**
	 * The owner and the sibling components that implement the interactable interface, owner first, gathered on activation.
	 */
	TArray<TWeakObjectPtr<UObject>> Handlers;

	/** The owner's Interactable Groups this belongs to, gathered on activation like the handlers, whose rules follow our own. */
	TArray<TWeakObjectPtr<UKzInteractableGroupComponent>> Groups;

	/** Calls Visit on each handler until it returns false; false when one did. */
	bool ForEachHandler(TFunctionRef<bool(UObject*)> Visit) const;

	/** Which rules a check asks about. */
	enum class ERuleCheck : uint8 { Hide, Block, End };

	/** The first rule of ours, then of our groups, that the check asks about and the interactor fails; null if none. */
	const FKzInteractionRule* FindFailedRule(UKzInteractorComponent* Interactor, ERuleCheck Check) const;
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
};