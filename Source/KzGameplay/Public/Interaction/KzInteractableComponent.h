// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "Components/KzComponentReference.h"
#include "Interaction/KzInteractionTypes.h"
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
	 * Logic requirement.
	 * e.g., "Does the player have the Key?", "Is the power On?"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	mutable FScriptableRequirement InteractionRequirement;

	/** Fired when the interaction has been successfully triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FScriptableAction InteractionAction;

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
	 * Runs the interaction against the owner and any sibling handlers, aggregating their answers.
	 * Called by the subsystem, which owns the interaction: do not call this directly, start one instead.
	 */
	virtual EKzInteractionResult ExecuteInteraction(UKzInteractorComponent* Interactor, const FKzInteraction& Interaction);

	/** Tells the owner and sibling handlers that an interaction ended. Called by the subsystem. */
	virtual void NotifyInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason);

	/** Asks the range rule and the handlers whether a running interaction still holds. Called by the subsystem. */
	virtual bool ShouldKeepInteractionAlive(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason) const;

	/** Ends every interaction running on this component. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopAllInteractions(EKzInteractionEndReason Reason = EKzInteractionEndReason::Interrupted);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
};