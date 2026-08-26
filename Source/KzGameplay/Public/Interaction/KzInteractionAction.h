// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzInteractionAction.generated.h"

namespace KzInteraction
{
	/**
	 * Declares the context an interaction container is authored against.
	 * Writes ContextDefinitions, which is the persisted shape. Filling only the transient bag is not
	 * enough: it is rebuilt from the definitions, so the editor would find nothing to bind against.
	 */
	KZGAMEPLAY_API void DeclareContext(FScriptableContainer& Container);
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

	/** Animation the instigator plays. Empty means no animation, and the effect lands immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FGameplayTag AnimationTag;

	/** Notify that marks the moment the effect lands. Empty runs the effect as the montage starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
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