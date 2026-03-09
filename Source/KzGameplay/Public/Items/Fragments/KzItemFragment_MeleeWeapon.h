// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Items/KzItemFragment.h"
#include "Core/KzDatabase.h" 
#include "Math/Geometry/KzShapeInstance.h"
#include "Misc/KzTransformSource.h"
#include "KzItemFragment_MeleeWeapon.generated.h"

/** Defines which mesh should be used as the base for the melee collision trace. */
UENUM(BlueprintType)
enum class EKzMeleeMeshTarget : uint8
{
	/** Uses the physical actor spawned by the equipment system (e.g., the Sword mesh). */
	Weapon,

	/** Uses the character's skeletal mesh (e.g., for punches, kicks, or unarmed combat). */
	Avatar
};

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

	// --- Collision Data ---

	/** Defines whether this step traces from the weapon's mesh or the character's body. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Collision")
	EKzMeleeMeshTarget MeshTarget = EKzMeleeMeshTarget::Weapon;

	/** The geometric shape to use for the melee sweep (e.g., Capsule for swords, Box for shields). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Collision")
	FKzShapeInstance TraceShape;

	/** The socket name on the resolved mesh to use as the center of the shape. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Collision")
	FName TraceSocketName = TEXT("b_melee_center");

	/** Local space offset from the socket to perfectly align the shape with the impact point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Collision")
	FTransform TraceOffset = FTransform::Identity;
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
	int32 GetNumComboSteps() const { return ComboSteps.Num(); }

	/** Returns true if this weapon has at least one combo step defined. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	bool HasCombos() const { return !ComboSteps.IsEmpty(); }

	/**
	 * Safely retrieves the combo step at the specified index.
	 * @param Index The index of the combo step to retrieve.
	 * @param OutStep The retrieved combo step data.
	 * @return True if the index was valid and the step was found, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	bool GetComboStep(int32 Index, FKzMeleeComboStep& OutStep) const;

	/**
	 * Calculates the final damage for a specific combo step (BaseDamage * Step.DamageMultiplier).
	 * If the index is invalid, it defaults to returning the BaseDamage.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	float CalculateDamageForStep(int32 Index) const;

	/**
	 * Safely calculates the next combo index based on the current sequence length.
	 * @param CurrentIndex The current combo step index.
	 * @param bLoop If true, the index wraps around to 0 when reaching the end. If false, it stops at the last index.
	 * @return The next valid combo index, or -1 if there are no combos defined.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	int32 GetNextComboIndex(int32 CurrentIndex, bool bLoop = true) const;

	/**
	 * Resolves the collision shape and transform source for a specific combo step.
	 * This encapsulates the logic of finding the correct mesh (Weapon vs Avatar) and applying sockets/offsets.
	 * @param Index The combo step index to evaluate.
	 * @param AvatarActor The character performing the attack. Used if the step targets the Avatar mesh.
	 * @param WeaponActor The physical weapon actor spawned in the world. Used if the step targets the Weapon mesh.
	 * @param OutShape The resolved collision shape ready to be passed to the Ability Task.
	 * @param OutTransformSource The resolved transform source ready to be passed to the Ability Task.
	 * @return True if the step was valid and the data was successfully resolved, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Melee")
	bool GetCollisionDataForStep(int32 Index, AActor* AvatarActor, AActor* WeaponActor, FKzShapeInstance& OutShape, FKzTransformSource& OutTransformSource) const;
};