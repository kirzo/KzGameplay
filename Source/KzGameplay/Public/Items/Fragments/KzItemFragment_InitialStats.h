// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Items/KzItemFragment.h"
#include "GameplayTagContainer.h"
#include "KzItemFragment_InitialStats.generated.h"

/**
 * A generic fragment that grants a set of default statistics to an item instance upon creation.
 * Examples: Initial durability, max ammo, charge level, etc.
 */
UCLASS(DisplayName = "Initial Stats")
class KZGAMEPLAY_API UKzItemFragment_InitialStats : public UKzItemFragment
{
	GENERATED_BODY()

public:

	/** The default stats to apply to the item instance when it is first created. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TMap<FGameplayTag, float> InitialStats;

	virtual void OnInstanceCreated(FKzItemInstance& Instance) const override;
};