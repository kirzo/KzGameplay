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

/** Defines how this item can be stored or handled by the character. */
UENUM(BlueprintType)
enum class EKzItemStorageMode : uint8
{
	/** The item goes to the backpack and cannot be equipped (e.g., Crafting materials). */
	InventoryOnly,

	/** The item must be equipped directly. It cannot be stored in the backpack (e.g., A water bucket, a heavy log). */
	EquipmentOnly,

	/** The item can be stored in the backpack or equipped in a slot (e.g., An axe, a flashlight). */
	Both
};

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
	// STORAGE & RULES
	// ==========================================

	/** Determines where this item is allowed to go (Backpack, Hands, or Both). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	EKzItemStorageMode StorageMode = EKzItemStorageMode::InventoryOnly;

	/** Maximum number of this item that can be stacked in a single inventory slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** The physical actor class to spawn if this item is dropped from the inventory into the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	TSoftClassPtr<AActor> WorldActorClass;

	// ==========================================
	// EQUIPMENT
	// ==========================================

	/** The specific equipment slot this item goes into (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	FGameplayTag TargetSlot;

	/** If true, the system will attempt to equip this item immediately upon picking it up, bypassing the backpack if possible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode == EKzItemStorageMode::Both", EditConditionHides))
	bool bAutoEquip = false;

	/**
	 * If true, the system will NOT use the default AttachToComponent.
	 * Instead, it will call PerformCustomAttach/Detach on the item's ItemComponent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	bool bUseCustomAttachment = false;

	/** Transform offset applied when the item is attached to an equipment socket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	FTransform AttachmentOffset;

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

	UFUNCTION(BlueprintCallable, Category = "Item|Equipment")
	FORCEINLINE bool IsEquippable() const { return StorageMode != EKzItemStorageMode::InventoryOnly; }

	UFUNCTION(BlueprintCallable, Category = "Item|Equipment")
	FORCEINLINE bool ShouldAutoEquip() const { return StorageMode == EKzItemStorageMode::EquipmentOnly || (IsEquippable() && bAutoEquip); }
};