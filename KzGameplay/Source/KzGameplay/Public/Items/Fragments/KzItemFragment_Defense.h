// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
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

	/** How long holder and attacker freeze when it blocks; zero does not freeze. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0", Units = "Seconds"))
	float BlockHitStop = 0.05f;

	/** How wide in front of its holder it parries; zero does not parry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0", ClampMax = "360", Units = "Degrees"))
	float ParryAngle = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (Categories = "GameplayCue"))
	FGameplayTag ParryCue;

	/** How long holder and attacker freeze when it parries; zero does not freeze. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0", Units = "Seconds"))
	float ParryHitStop = 0.1f;

	/** How fast the parried attacker moves afterwards, leaving the holder a moment to strike back; one does not slow it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ParrySlowDilation = 0.3f;

	/** How long the parried attacker stays slow, freeze included. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0", Units = "Seconds"))
	float ParrySlowDuration = 0.6f;

	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const override;
	virtual void GetEquippedTags(FGameplayTagContainer& OutTags) const override;
};