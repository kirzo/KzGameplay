// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableContext.h"
#include "Interaction/KzInteractionTypes.h"
#include "KzInteractionAction.generated.h"

class UKzInteractorComponent;
class UKzInteractableComponent;

namespace KzInteraction
{
	/**
	 * Declares the context an interaction container is authored against.
	 * Writes ContextDefinitions, which is the persisted shape. Filling only the transient bag is not
	 * enough: it is rebuilt from the definitions, so the editor would find nothing to bind against.
	 */
	KZGAMEPLAY_API void DeclareContext(FScriptableContainer& Container);

	/**
	 * Fills that same context. Kept next to DeclareContext on purpose: the shape and its values have to
	 * agree, and they only do if adding an entry means editing two adjacent functions instead of hunting
	 * down every place that fills one.
	 */
	KZGAMEPLAY_API void FillContext(FScriptableContainer& Container, UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);

	/** That same context as values, for running a copy of a container declared with DeclareContext. */
	KZGAMEPLAY_API FScriptableContext MakeContext(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);

	/** Declares the context of what runs when an interaction ends: the interaction context plus Reason and Detail. */
	KZGAMEPLAY_API void DeclareEndContext(FScriptableContainer& Container);

	/** The end context as values, for running a copy of a container declared with DeclareEndContext. */
	KZGAMEPLAY_API FScriptableContext MakeEndContext(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason, FGameplayTag Detail);
}

/**
 * Something the instigator can do repeatedly while an interaction is running: press a button, play an
 * animation, and have an effect land on one of its notifies.
 *
 * The animation is named, not referenced: the object says what it wants played and the avatar answers with
 * whatever montage fits its own skeleton, the same way an object asks for a capability rather than an ability.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzInteractionAction
{
	GENERATED_BODY()

	/**
	 * Declares the context of both containers. An array element has no owning constructor to do it for
	 * it, and the shape must exist before runtime for the bindings to be authorable at all.
	 */
	FKzInteractionAction();

	/** Input that fires it while the interaction is running. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (Categories = "Input"))
	FGameplayTag InputTag;

	/**
	 * The object plays the action itself, as a lever its timeline, in place of an animation of the instigator: the press only starts it,
	 * through the interactable's OnActionStarted, and the object calls Land Action when the effect is due and Finish Action once it is over.
	 * Those pace the presses, so there are no times to keep in step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	bool bPerformedByInteractable = false;

	/** Animation the instigator plays. Empty means no animation, and the effect lands immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (EditCondition = "!bPerformedByInteractable", EditConditionHides))
	FGameplayTag AnimationTag;

	/** Notify that marks the moment the effect lands. Empty runs the effect as the montage starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (EditCondition = "!bPerformedByInteractable", EditConditionHides))
	FGameplayTag EffectNotify;

	/**
	 * Rules for this action alone. The interactable's own requirements already decided whether the
	 * interaction could start, so this is about whether this particular action can run right now.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	mutable FScriptableRequirement Requirement;

	/** What actually happens. Runs everywhere the ability runs, so guard authoritative work inside it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FScriptableAction Effect;

	/** Minimum seconds between uses. 0 leaves the pacing to the animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ClampMin = 0.0))
	float Cooldown = 0.0f;

	/** Shown on the prompt while this action is available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FText PromptText;
};