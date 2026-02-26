// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Items/ItemInstance.h"
#include "Equipment/EquipmentLayout.h"
#include "EquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChangedDelegate, FGameplayTag, SlotID, const FItemInstance&, NewItem);

/**
 * Represents a specific slot in the equipment system and the item currently in it.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FEquippedSlot
{
	GENERATED_BODY()

public:
	/** The tag identifying this slot (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FGameplayTag SlotID;

	/** The item currently equipped in this slot. If Quantity is 0, the slot is empty. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FItemInstance Instance;

	FEquippedSlot() {}
	FEquippedSlot(FGameplayTag InSlotID) : SlotID(InSlotID) {}
};

/**
 * Manages active equipment slots for a character.
 * Handles the logic of swapping items, replicating the equipped state, and triggering attachment events.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	/** Delegate fired whenever an item is equipped or unequipped. */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChangedDelegate OnEquipmentChanged;

	/** The default layout defining which slots this character has available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Configuration")
	TObjectPtr<const UEquipmentLayout> DefaultLayout;

	/** * Initializes the empty slots based on the provided layout.
	 * Usually called automatically on BeginPlay using DefaultLayout.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void InitializeEquipment(const UEquipmentLayout* Layout);

	/** * Attempts to equip an item into its designated target slot.
	 * If the slot was already occupied, the old item is returned via OutUnequippedItem.
	 * @return True if the item was successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool EquipItem(const FItemInstance& ItemToEquip, FItemInstance& OutUnequippedItem);

	/**
	 * Equips an item directly from an ItemComponent in the world.
	 * Triggers pre-pickup events before attaching.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemFromWorld(class UItemComponent* ItemComp, FItemInstance& OutUnequippedItem);

	/** * Removes whatever item is currently in the specified slot.
	 * @return True if there was an item to unequip.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool UnequipItem(FGameplayTag SlotID, FItemInstance& OutUnequippedItem);

	/** Returns the item instance currently occupying the given slot, if any. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FItemInstance GetItemInSlot(FGameplayTag SlotID) const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Replicated array holding the current state of all equipment slots. */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedSlots)
	TArray<FEquippedSlot> EquippedSlots;

	UFUNCTION()
	void OnRep_EquippedSlots(const TArray<FEquippedSlot>& OldEquippedSlots);
};