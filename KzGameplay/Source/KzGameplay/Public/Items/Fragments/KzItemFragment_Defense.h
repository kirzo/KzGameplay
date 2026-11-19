// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Combat/KzCombatTypes.h"
#include "GameplayTagContainer.h"
#include "Items/KzItemFragment.h"
#include "KzItemFragment_Defense.generated.h"

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

	/** How long holder and attacker freeze when it blocks, and how they come back to speed; zero does not freeze. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	FKzHitStop BlockHitStop = FKzHitStop(0.05f);

	/** How wide in front of its holder it parries; zero does not parry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0", ClampMax = "360", Units = "Degrees"))
	float ParryAngle = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (Categories = "GameplayCue"))
	FGameplayTag ParryCue;

	/** How long holder and attacker freeze when it parries, and how they come back to speed; zero does not freeze. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	FKzHitStop ParryHitStop = FKzHitStop(0.1f);

	/** How the parried attacker moves afterwards, freeze included, leaving the holder a moment to strike back: 0.3 of its speed for 0.6 s unless changed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	FKzHitStop ParrySlow = FKzHitStop(0.6f);

	UKzItemFragment_Defense();

	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const override;
	virtual void GetEquippedTags(FGameplayTagContainer& OutTags) const override;
};