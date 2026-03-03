// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzItemDefinition.generated.h"

class AActor;
class UStreamableRenderAsset;
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

/** Defines how the item is represented visually when equipped. */
UENUM(BlueprintType)
enum class EKzEquipmentSpawnMode : uint8
{
	/** Spawns a full Actor (Uses WorldActorClass or EquipmentActorClass). */
	SpawnActor,

	/** Only creates a Mesh Component (Uses EquipmentMesh). */
	SpawnMesh
};

/**
 * Defines the core, immutable data and rules for an item in the game.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UKzItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UKzItemDefinition();

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

	/** The physical actor class to spawn if this item is dropped from the inventory into the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	TSoftClassPtr<AActor> WorldActorClass;

	/**
	 * Gameplay Tags describing this item's properties or categories.
	 * e.g., "Item.Weapon.Axe", "Item.Throwable", "Item.Material.Wood".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	FGameplayTagContainer ItemTags;

	// ==========================================
	// INVENTORY
	// ==========================================

	/** Maximum number of this item that can be stacked in a single inventory slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1", EditCondition = "StorageMode != EKzItemStorageMode::EquipmentOnly", EditConditionHides))
	int32 MaxStackSize = 1;

	/**
	 * Tags automatically granted to the Character's Ability System while this item is in the inventory.
	 * The tag count will increase based on the quantity of this item held.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (EditCondition = "StorageMode != EKzItemStorageMode::EquipmentOnly", EditConditionHides))
	FGameplayTagContainer InventoryTags;

	// ==========================================
	// EQUIPMENT
	// ==========================================

	/** If true, the system will attempt to equip this item immediately upon picking it up, bypassing the backpack if possible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode == EKzItemStorageMode::Both", EditConditionHides))
	bool bAutoEquip = false;

	/** The specific equipment slot this item goes into (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	FGameplayTag TargetSlot;

	/**
	 * If true, the system will NOT use the default AttachToComponent.
	 * Instead, it will call PerformCustomAttach/Detach on the item's ItemComponent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	bool bUseCustomAttachment = false;

	/** How this item should be instantiated when equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	EKzEquipmentSpawnMode EquipmentSpawnMode = EKzEquipmentSpawnMode::SpawnActor;

	/** If true, uses a different Actor class when equipped instead of the default WorldActorClass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly && EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnActor", EditConditionHides))
	bool bOverrideEquipmentActor = false;

	/** The specialized actor class to spawn when equipped (e.g., a weapon with shooting logic). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly && EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnActor && bOverrideEquipmentActor", EditConditionHides))
	TSoftClassPtr<AActor> EquipmentActorClass;

	/** The mesh to attach to the character. Can be a UStaticMesh or USkeletalMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly && EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh", EditConditionHides, AllowedClasses = "/Script/Engine.StaticMesh, /Script/Engine.SkeletalMesh"))
	TSoftObjectPtr<UStreamableRenderAsset> EquipmentMesh;

	/** Transform offset applied when the item is attached to an equipment socket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	FTransform AttachmentOffset;

	/** Tags automatically granted to the Owner's Ability System while this item is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	FGameplayTagContainer EquippedTags;

	/** Helper to get the correct class to spawn when equipping as an actor. */
	UFUNCTION(BlueprintCallable, Category = "Item|Equipment")
	TSoftClassPtr<AActor> GetEquippedActorClass() const
	{
		return bOverrideEquipmentActor ? EquipmentActorClass : WorldActorClass;
	}

	// ==========================================
	// EVENTS
	// ==========================================

	/** Action to execute when this item is added to the inventory (e.g., granting a passive buff). */
	UPROPERTY(EditAnywhere, Category = "Events", meta = (EditCondition = "StorageMode != EKzItemStorageMode::EquipmentOnly", EditConditionHides))
	FScriptableAction OnAcquiredAction;

	/** Action to execute when the item is equipped (e.g., granting an active ability). */
	UPROPERTY(EditAnywhere, Category = "Events", meta = (EditCondition = "StorageMode != EKzItemStorageMode::InventoryOnly", EditConditionHides))
	FScriptableAction OnEquippedAction;

	UFUNCTION(BlueprintCallable, Category = "Item|Equipment")
	FORCEINLINE bool IsEquippable() const { return StorageMode != EKzItemStorageMode::InventoryOnly; }

	UFUNCTION(BlueprintCallable, Category = "Item|Equipment")
	FORCEINLINE bool ShouldAutoEquip() const { return StorageMode == EKzItemStorageMode::EquipmentOnly || (IsEquippable() && bAutoEquip); }
};