// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableConditions/ScriptableCondition.h"
#include "GameplayTagContainer.h"
#include "ScriptableCondition_HasCapability.generated.h"

/**
 * Checks whether the target can do something, which is the question the world should be asking.
 *
 * Reads the capability tag off the ability system rather than going through UKzCapabilityComponent, so it
 * also answers for actors that were given the tag by other means, and on clients, where the tag replicates
 * but the granting component does nothing.
 */
UCLASS(DisplayName = "Has Capability", meta = (ConditionCategory = "Gameplay|Capabilities"))
class KZGAMEPLAY_API UScriptableCondition_HasCapability : public UScriptableCondition
{
	GENERATED_BODY()

protected:
	/** The actor being asked. */
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ScriptableContext))
	TObjectPtr<AActor> TargetActor;

	/** The capability the actor must have. */
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (Categories = "Capability"))
	FGameplayTag Capability;

	/**
	 * Whether having it is not enough and it has to be usable this instant, which also asks whatever
	 * implements the capability. Use it for what the player should see blocked and understand: you can
	 * push, but not while your hands are full.
	 *
	 * Needs a UKzCapabilityComponent on the target to answer; without one it falls back to the plain check.
	 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bMustBeUsableNow = false;

protected:
	virtual bool Evaluate_Implementation() const override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};