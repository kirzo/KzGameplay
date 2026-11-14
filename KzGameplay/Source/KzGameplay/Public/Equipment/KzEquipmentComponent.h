// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Items/KzItemInstance.h"
#include "Equipment/KzEquipmentLayout.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzEquipmentComponent.generated.h"

class UKzEquipmentComponent;
class UKzInteractableComponent;
class UKzItemFragment_Equippable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChangedDelegate, FGameplayTag, SlotID, const FKzItemInstance&, Item);

struct FKzEquipmentList;

/**
 * Represents a specific slot in the equipment system and the item currently in it.
 * A FastArray item, so equipment replicates per-slot deltas and drives local visuals on each machine.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FEquippedSlot : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	/** The tag identifying this slot (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FGameplayTag SlotID;

	/** The item currently equipped in this slot. If Quantity is 0, the slot is empty. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FKzItemInstance Instance;

	/** The interactable the item was picked up by, null when it was handed over: which part of an item held in several slots this one holds. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	TObjectPtr<UKzInteractableComponent> PickedUpBy = nullptr;

	/** The item this machine shows in the slot and told its equip listeners about. Local: by the time a change replicates, clients have lost the old item. */
	UPROPERTY(NotReplicated, Transient)
	TObjectPtr<const UKzItemDefinition> ShownItem = nullptr;

	/** The item actor this machine has the slot holding, told so through its ItemComponent. Local, for the same reason as ShownItem. */
	UPROPERTY(NotReplicated, Transient)
	TWeakObjectPtr<AActor> HeldActor;

	FEquippedSlot() {}
	FEquippedSlot(FGameplayTag InSlotID) : SlotID(InSlotID) {}

	/** Equality operator against another slot. */
	bool operator==(const FEquippedSlot& Other) const
	{
		return SlotID == Other.SlotID;
	}

	/** Equality operator against a GameplayTag (Allows using TArray::FindByKey). */
	bool operator==(FGameplayTag OtherTag) const
	{
		return SlotID == OtherTag;
	}

	// FastArray client-side callbacks.
	void PostReplicatedAdd(const FKzEquipmentList& InArraySerializer);
	void PostReplicatedChange(const FKzEquipmentList& InArraySerializer);
	void PreReplicatedRemove(const FKzEquipmentList& InArraySerializer);
};

/**
 * FastArray of equipment slots, giving per-slot delta replication and client callbacks.
 */
USTRUCT()
struct KZGAMEPLAY_API FKzEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	/** The replicated slots. */
	UPROPERTY()
	TArray<FEquippedSlot> Slots;

	/** Owning component, used by slot callbacks. Set on construction; not replicated. */
	UPROPERTY(NotReplicated)
	TObjectPtr<UKzEquipmentComponent> OwnerComponent = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FEquippedSlot, FKzEquipmentList>(Slots, DeltaParms, *this);
	}
};

/** Boilerplate required to tell the engine this struct has a custom net delta serializer. */
template<>
struct TStructOpsTypeTraits<FKzEquipmentList> : public TStructOpsTypeTraitsBase2<FKzEquipmentList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * Manages active equipment slots for a character.
 * Handles the logic of swapping items, replicating the equipped state, and triggering attachment events.
 *
 * The slots are the truth of who holds what: the server changes them, they replicate, and every machine applies them to the item actors
 * they hold, attaching them and telling their ItemComponents. An item actor can be in slots of several equipments at once, and in several
 * of one, up to its Equippable's Max Holders.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FEquippedSlot;

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

	/** Equipped on BeginPlay, each into its Equippable's slot. Needs a DefaultLayout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<TObjectPtr<const UKzItemDefinition>> DefaultItems;

	/** * Initializes the empty slots based on the provided layout.
	 * Usually called automatically on BeginPlay using DefaultLayout.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void InitializeEquipment(const UKzEquipmentLayout* Layout);

	/** * Attempts to equip an item into its designated target slot, or the slot FindSlotFor gives when that one already holds it.
	 * If the slot was already occupied, the old item is returned via OutUnequippedItem.
	 * @param PickedUpBy The interactable of the item's actor it was picked up by, which the slot keeps: null when it is handed over.
	 * @return True if the item was successfully equipped. False, without touching any slot, if no more slots can hold its actor.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool EquipItem(const FKzItemInstance& ItemToEquip, FKzItemInstance& OutUnequippedItem, UKzInteractableComponent* PickedUpBy = nullptr);

	/**
	 * Equips an item directly from its definition.
	 * Useful for starting gear, granting items via code/Blueprint, or equipping from an inventory.
	 * @return True if the item was successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool EquipItemByDefinition(const UKzItemDefinition* ItemDef, FKzItemInstance& OutUnequippedItem);

	/**
	 * Equips an item directly from an KzItemComponent in the world.
	 * Triggers pre-pickup events before attaching.
	 * @param PickedUpBy The interactable it was picked up by, null when it was not.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemFromWorld(class UKzItemComponent* ItemComp, FKzItemInstance& OutUnequippedItem, UKzInteractableComponent* PickedUpBy = nullptr);

	/**
	 * The slot equipping an item would put it in: its Target Slot, or, when that already holds this very actor, the first of its Extra Slots
	 * that does not. Invalid when none of them is left, or without a layout.
	 * @param ItemActor The item's actor, null for an item equipped as a mesh.
	 */
	FGameplayTag FindSlotFor(const UKzItemDefinition* ItemDef, const AActor* ItemActor) const;

	/** * Removes whatever item is currently in the specified slot.
	 * An item still held by other slots stays with them; let go by its last one, it goes to the inventory or drops.
	 * @return True if there was an item to unequip.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool UnequipItem(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag SlotID, FKzItemInstance& OutUnequippedItem);

	/**
	 * Puts the item in SlotID away in its Equippable's Stow Slot, still equipped: what SlotID granted goes away until it is drawn.
	 * @return True if there was an item to put away and room for it.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool StowItem(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag SlotID);

	/**
	 * Takes the item put away from SlotID back into it.
	 * @return True if there was one and SlotID was free.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool DrawItem(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag SlotID);

	/**
	 * Returns a read-only pointer to the item instance currently occupying the given slot, if any.
	 * @return Pointer to the instance, or nullptr if the slot is empty or invalid.
	 */
	const FKzItemInstance* FindItemInSlot(FGameplayTag SlotID) const;

	/** Every slot and what it holds. */
	TConstArrayView<FEquippedSlot> GetSlots() const { return EquipmentList.Slots; }

	/** Returns a copy of the item instance currently occupying the given slot, if any. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FKzItemInstance GetItemInSlot(UPARAM(meta = (Categories = "Equipment.Slot")) FGameplayTag SlotID) const;

	/** Resolves what items in a slot hang from: the slot AttachComponent, or the owner skeletal mesh when it is empty. */
	USceneComponent* GetAttachParentForSlot(FGameplayTag SlotID) const;

	/** Where equipping would hang an item now: the component and socket of the slot it would go to, its own socket first. Null without a layout. */
	USceneComponent* FindAttachmentForItem(const UKzItemFragment_Equippable& Equippable, FName& OutSocket) const;

	/** Whether the slot holds its item put away: the slot is the item's Stow Slot, not its Target Slot. */
	bool IsStowed(const FEquippedSlot& Slot, const UKzItemFragment_Equippable& Equippable) const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Lets go of the item actors its slots hold on this machine: a character that goes stops holding them everywhere. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Replicated FastArray holding the current state of all equipment slots. */
	UPROPERTY(Replicated)
	FKzEquipmentList EquipmentList;

	/** Capabilities and tags this component granted, so a refresh takes back only what it gave. */
	TSet<FGameplayTag> ContributedCapabilities;
	TSet<FGameplayTag> ContributedTags;

	/** The layout the slots were built from, kept because slots carry rules and not just names. */
	UPROPERTY(Transient)
	TObjectPtr<const UKzEquipmentLayout> ActiveLayout;

	/** The layout in force: the one it was initialized with, or the default on machines that never ran InitializeEquipment. */
	const UKzEquipmentLayout* GetLayout() const;

	/** Moves an item, actor and all, between its slot and its stow slot; the visuals and grants follow. */
	void MoveItem(FEquippedSlot& From, FEquippedSlot& To);

	/** Creates the local cosmetic mesh for a slot on the current machine, replacing any existing one: the item's in SpawnMesh mode, or its Stowed Mesh when put away. */
	void RefreshVisualForSlot(FEquippedSlot& Slot);

	/** Destroys the local cosmetic mesh for a slot on the current machine. */
	void ClearVisualForSlot(FEquippedSlot& Slot);

	/** Makes ItemDef what this machine shows in the slot, telling the old item's equip listeners it went and the new one's it came. An item put away counts as none. Dedicated servers show nothing. */
	void ShowItem(FEquippedSlot& Slot, const UKzItemDefinition* ItemDef);

	/**
	 * Makes the slot's item actor the one this machine has the slot holding: lets go of the one it held, and attaches the new one, or lets it
	 * attach itself, telling its ItemComponent. Every machine runs it as the slot changes, the server as it changes it.
	 */
	void HoldItemActor(FEquippedSlot& Slot);

	/** Lets go of the actor this machine has the slot holding. Let go by every slot, it is a thing of the world again: it collides, and falls if it simulates. */
	void ReleaseItemActor(FEquippedSlot& Slot);

	/** Where the slot holds its item: its component, socket and offset, as the item's Equippable says for that slot, put away or in use. */
	USceneComponent* GetAttachmentForSlot(const FEquippedSlot& Slot, const UKzItemFragment_Equippable& Equippable, FName& OutSocket, FTransform& OutOffset) const;

	/** Called on clients when a slot first replicates in: refresh the visual and broadcast if it holds an item. */
	void HandleSlotAdded(FEquippedSlot& Slot);

	/** Called on clients when a slot's item changes: refresh the visual and broadcast equip/unequip. */
	void HandleSlotUpdated(FEquippedSlot& Slot);

	/** Called on clients when a slot replicates out: drop the visual and put its item away. */
	void HandleSlotRemoved(FEquippedSlot& Slot);

	/**
	 * Works out what the occupied slots and their items add up to, capabilities and tags alike, and
	 * grants or revokes the difference.
	 *
	 * Recomputed rather than counted: two hands can contribute the same thing, and emptying one of them
	 * must not take it from the other. The whole answer is cheap and cannot drift.
	 */
	void RefreshEquippedGrants();
};