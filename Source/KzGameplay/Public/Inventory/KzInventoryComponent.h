// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/KzItemInstance.h"
#include "KzInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedDelegate);

class UKzItemDefinition;

/**
 * A highly modular, networked inventory component that stores Item Instances.
 * Responsible for managing capacity, stacking, and executing Scriptable Actions upon item acquisition.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzInventoryComponent();

	/** Delegate fired whenever the inventory contents change (useful for updating UI). */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChangedDelegate OnInventoryChanged;

	/** Total number of unique slots available in this backpack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Configuration", meta = (ClampMin = "1"))
	int32 Capacity;

	/**
	 * Attempts to add an item to the inventory.
	 * If successful, the PhysicalActor (if valid) will be destroyed as it enters the backpack.
	 * @return True if at least some quantity of the item was added.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool TryAddItem(const UKzItemDefinition* ItemDef, int32 Quantity, AActor* PhysicalActor = nullptr);

	/** Attempts to remove a specific quantity of an item from the inventory. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool RemoveItem(const UKzItemDefinition* ItemDef, int32 Quantity);

	/** Checks if there is enough room for a new item stack. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasSpace() const;

	/** Returns the current list of items. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FKzItemInstance>& GetItems() const { return Items; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The replicated list of items currently held in this inventory. */
	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FKzItemInstance> Items;

	/** Called on clients whenever the Server updates the Items array. */
	UFUNCTION()
	virtual void OnRep_Items();

	/** Internal helper to find an existing stack that isn't full. */
	int32 FindStackableSlot(const UKzItemDefinition* ItemDef) const;
};