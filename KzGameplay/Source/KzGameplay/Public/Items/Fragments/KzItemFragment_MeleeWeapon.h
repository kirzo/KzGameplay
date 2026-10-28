// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Items/KzItemFragment.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "KzItemFragment_MeleeWeapon.generated.h"

class UGameplayEffect;
class UKzComboGraph;

/** A weapon swung in melee: the combo it attacks with and its damage. */
UCLASS(ClassGroup = "Weapons", DisplayName = "Melee Weapon")
class KZGAMEPLAY_API UKzItemFragment_MeleeWeapon : public UKzItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TSoftObjectPtr<UKzComboGraph> Combo;

	/** Damage per hit, before each attack's multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee", meta = (ClampMin = "0"))
	float BaseDamage = 25.0f;

	/** Applied to whatever it hits, with the damage as the Set By Caller magnitude Data.Damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** A melee weapon is swung from the hands, so it must be equippable. */
	virtual void GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const override;
	virtual void GetPreloadPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};