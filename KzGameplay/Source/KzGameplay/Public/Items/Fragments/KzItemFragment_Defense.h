// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Combat/KzCombatTypes.h"
#include "GameplayTagContainer.h"
#include "Items/KzItemFragment.h"
#include "KzItemFragment_Defense.generated.h"

class UKzHitStop;

/**
 * An item its holder defends with, such as a shield: without one equipped, State.Blocking and State.Parrying stop nothing.
 * While equipped it gives Equipment.Defense.Block and Equipment.Defense.Parry, for the abilities that need them.
 */
UCLASS(ClassGroup = "Weapons", DisplayName = "Defense")
class KZGAMEPLAY_API UKzItemFragment_Defense : public UKzItemFragment
{
	GENERATED_BODY()

public:
	/** How wide in front of its holder it blocks; zero does not block. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0", ClampMax = "360", Units = "Degrees"))
	float BlockAngle = 120.0f;

	/** Executed on the holder when it blocks, at the item: sparks, a clang. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block", meta = (Categories = "GameplayCue"))
	FGameplayTag BlockCue;

	/** Executed when a Guard Break attack breaks the block; none uses the Block Cue. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block", meta = (Categories = "GameplayCue"))
	FGameplayTag GuardBreakCue;

	/** How holder and attacker freeze when it blocks; none, they don't. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	TObjectPtr<const UKzHitStop> BlockHitStop;

	/** How wide in front of its holder it parries; zero does not parry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0", ClampMax = "360", Units = "Degrees"))
	float ParryAngle = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (Categories = "GameplayCue"))
	FGameplayTag ParryCue;

	/** How holder and attacker freeze when it parries; none, they don't. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	TObjectPtr<const UKzHitStop> ParryHitStop;

	/**
	 * How the parried attacker moves afterwards, leaving the holder a moment to strike back, such as a Hit Stop on the Instigator that holds it at 0.7
	 * for 0.6 s. Alongside Parry Hit Stop, the slower of the two holds it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	TObjectPtr<const UKzHitStop> ParrySlow;

	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const override;
	virtual void GetEquippedTags(FGameplayTagContainer& OutTags) const override;
};