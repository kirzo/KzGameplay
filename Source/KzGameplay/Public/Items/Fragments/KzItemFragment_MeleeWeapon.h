// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Items/KzItemFragment.h"
#include "Core/KzDatabase.h" 
#include "KzItemFragment_MeleeWeapon.generated.h"

/** Defines a single step in a melee attack combo, using semantic queries for animation retrieval. */
USTRUCT(BlueprintType)
struct FKzMeleeComboStep
{
	GENERATED_BODY()

	/**
	 * The query used to find the correct animation in the target's internal database.
	 * e.g., RequireTags: "Combat.Melee.Axe.Light1".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	FKzDatabaseQuery AnimationQuery;

	/** Damage multiplier applied to the weapon's base damage for this specific hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float DamageMultiplier = 1.0f;
};

/** Defines generic melee combat data for an item. */
UCLASS(DisplayName = "Melee Weapon")
class KZGAMEPLAY_API UKzItemFragment_MeleeWeapon : public UKzItemFragment
{
	GENERATED_BODY()

public:

	/** The base damage applied per hit before any multipliers. */
	UPROPERTY(EditDefaultsOnly, BLueprintReadOnly, Category = "Melee")
	float BaseDamage = 25.0f;

	/** The sequence of attacks that make up the combo. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee", meta = (TitleProperty = "DamageMultiplier"))
	TArray<FKzMeleeComboStep> ComboSteps;

	/** Returns the total number of steps in this weapon's combo sequence. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	int32 GetNumComboSteps() const
	{
		return ComboSteps.Num();
	}

	/** Returns true if this weapon has at least one combo step defined. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	bool HasCombos() const
	{
		return !ComboSteps.IsEmpty();
	}

	/** * Safely retrieves the combo step at the specified index.
	 * @param Index The index of the combo step to retrieve.
	 * @param OutStep The retrieved combo step data.
	 * @return True if the index was valid and the step was found, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	bool GetComboStep(int32 Index, FKzMeleeComboStep& OutStep) const
	{
		if (ComboSteps.IsValidIndex(Index))
		{
			OutStep = ComboSteps[Index];
			return true;
		}
		return false;
	}

	/**
	 * Calculates the final damage for a specific combo step (BaseDamage * Step.DamageMultiplier).
	 * If the index is invalid, it defaults to returning the BaseDamage.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	float CalculateDamageForStep(int32 Index) const
	{
		if (ComboSteps.IsValidIndex(Index))
		{
			return BaseDamage * ComboSteps[Index].DamageMultiplier;
		}
		return BaseDamage;
	}

	/**
	 * Safely calculates the next combo index based on the current sequence length.
	 * @param CurrentIndex The current combo step index.
	 * @param bLoop If true, the index wraps around to 0 when reaching the end. If false, it stops at the last index.
	 * @return The next valid combo index, or -1 if there are no combos defined.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	int32 GetNextComboIndex(int32 CurrentIndex, bool bLoop = true) const
	{
		if (ComboSteps.IsEmpty())
		{
			return INDEX_NONE;
		}

		int32 NextIndex = CurrentIndex + 1;

		if (NextIndex >= ComboSteps.Num())
		{
			return bLoop ? 0 : (ComboSteps.Num() - 1);
		}

		return NextIndex;
	}
};