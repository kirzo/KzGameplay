// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Items/KzItemFragment.h"
#include "GameplayTagContainer.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzItemFragment_Storable.generated.h"

class UKzInventoryComponent;

/** Defines the rules for storing this item in a backpack/inventory. */
UCLASS(ClassGroup = "Inventory", DisplayName = "Storable")
class KZGAMEPLAY_API UKzItemFragment_Storable : public UKzItemFragment
{
	GENERATED_BODY()

public:
	UKzItemFragment_Storable();

	/** Maximum number of this item that can be stacked in a single inventory slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/**
	 * Tags automatically granted to the Character's Ability System while this item is in the inventory.
	 * The tag count will increase based on the quantity of this item held.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FGameplayTagContainer InventoryTags;

	/** Action to execute when this item is added to the inventory (e.g., granting a passive buff). */
	UPROPERTY(EditAnywhere, Category = "Events")
	FScriptableAction OnAcquiredAction;
};