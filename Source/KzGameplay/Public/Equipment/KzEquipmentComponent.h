// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Items/KzItemInstance.h"
#include "Equipment/KzEquipmentLayout.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzEquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChangedDelegate, FGameplayTag, SlotID, const FKzItemInstance&, Item);

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
	FKzItemInstance Instance;

	FEquippedSlot() {}
	FEquippedSlot(FGameplayTag InSlotID) : SlotID(InSlotID) {}
};

/**
 * Manages active equipment slots for a character.
 * Handles the logic of swapping items, replicating the equipped state, and triggering attachment events.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzEquipmentComponent();

	/** Delegate fired when an item is placed into a slot. */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChangedDelegate OnItemEquipped;

	/** Delegate fired when an item is removed from a slot. */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChangedDelegate OnItemUnequipped;

	/** Action executed whenever ANY item is successfully equipped. Useful for generic SFX/VFX. */
	UPROPERTY(EditAnywhere, Category = "Equipment|Events")
	FScriptableAction OnItemEquippedAction;

	/** Action executed whenever ANY item is successfully unequipped. Useful for generic SFX/VFX. */
	UPROPERTY(EditAnywhere, Category = "Equipment|Events")
	FScriptableAction OnItemUnequippedAction;

	/** The default layout defining which slots this character has available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<const UKzEquipmentLayout> DefaultLayout;

	/** * Initializes the empty slots based on the provided layout.
	 * Usually called automatically on BeginPlay using DefaultLayout.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void InitializeEquipment(const UKzEquipmentLayout* Layout);

	/** * Attempts to equip an item into its designated target slot.
	 * If the slot was already occupied, the old item is returned via OutUnequippedItem.
	 * @return True if the item was successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool EquipItem(const FKzItemInstance& ItemToEquip, FKzItemInstance& OutUnequippedItem);

	/**
	 * Equips an item directly from an KzItemComponent in the world.
	 * Triggers pre-pickup events before attaching.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemFromWorld(class UKzItemComponent* ItemComp, FKzItemInstance& OutUnequippedItem);

	/** * Removes whatever item is currently in the specified slot.
	 * @return True if there was an item to unequip.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool UnequipItem(FGameplayTag SlotID, FKzItemInstance& OutUnequippedItem);

	/** Returns the item instance currently occupying the given slot, if any. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FKzItemInstance GetItemInSlot(FGameplayTag SlotID) const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Replicated array holding the current state of all equipment slots. */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedSlots)
	TArray<FEquippedSlot> EquippedSlots;

	UFUNCTION()
	void OnRep_EquippedSlots(const TArray<FEquippedSlot>& OldEquippedSlots);
};