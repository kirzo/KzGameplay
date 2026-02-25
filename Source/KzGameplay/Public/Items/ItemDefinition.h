// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ItemDefinition.generated.h"

class AActor;
class UTexture2D;
class UScriptableAction;

/**
 * Defines the core, immutable data and rules for an item in the game.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UItemDefinition();

	// ==========================================
	// VISUALS & UI
	// ==========================================

	/** The localized name of the item to display in UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FText DisplayName;

	/** The localized description of the item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals", meta = (MultiLine = true))
	FText Description;

	/** The icon used in inventories or HUDs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	// ==========================================
	// RULES
	// ==========================================

	/** Maximum number of this item that can be stacked in a single inventory slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** The physical actor class to spawn if this item is dropped from the inventory into the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	TSoftClassPtr<AActor> WorldActorClass;

	// ==========================================
	// EQUIPMENT
	// ==========================================

	/** Can this item be equipped? If false, it acts strictly as an inventory/storage item (like a buff amulet or crafting material). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bIsEquippable;

	/** The specific equipment slot this item goes into (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "bIsEquippable", EditConditionHides))
	FGameplayTag TargetSlot;

	/** If true, the system will attempt to equip this item immediately upon picking it up, bypassing the backpack if possible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "bIsEquippable", EditConditionHides))
	bool bAutoEquip;

	// ==========================================
	// EVENTS
	// ==========================================

	/** Action to execute when this item is added to the inventory (e.g., granting a passive buff). */
	UPROPERTY(EditAnywhere, Category = "Events|Inventory")
	FScriptableAction OnAcquiredAction;

	/** Action to execute when this item is permanently removed from the inventory/dropped. */
	UPROPERTY(EditAnywhere, Category = "Events|Inventory")
	FScriptableAction OnRemovedAction;

	/** Action to execute when the item is equipped (e.g., granting an active ability). */
	UPROPERTY(EditAnywhere, Category = "Events|Equipment", meta = (EditCondition = "bIsEquippable", EditConditionHides))
	FScriptableAction OnEquippedAction;

	/** Action to execute when the item is unequipped and sent back to the backpack. */
	UPROPERTY(EditAnywhere, Category = "Events|Equipment", meta = (EditCondition = "bIsEquippable", EditConditionHides))
	FScriptableAction OnUnequippedAction;
};