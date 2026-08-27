// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Items/KzItemFragment.h"
#include "KzEquipmentLayout.generated.h"

/**
 * Defines a single equipment slot available for a character/entity.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzEquipmentSlotDefinition
{
	GENERATED_BODY()

public:
	/** The unique Gameplay Tag identifying this slot (e.g., Equipment.Slot.Head). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FGameplayTag SlotID;

	/** The default bone/socket name on the skeletal mesh where items in this slot should attach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FName DefaultSocketName;

	/** The localized name to display in the UI for this slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FText DisplayName;

	/**
	 * Capabilities the owner gains while this slot holds something, whatever it is holding.
	 * Dropping and throwing belong to a hand rather than to any particular object, and a head slot
	 * grants neither. What each capability turns into is the capability set's business.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot", meta = (Categories = "Capability"))
	FGameplayTagContainer GrantedCapabilities;

	/**
	 * Tags the owner carries while this slot holds something. Having a hand full is a fact about the
	 * hand, not about what is in it, so State.Grabbing belongs here rather than on every object.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FGameplayTagContainer EquippedTags;

	/**
	 * Capabilities granted only when what this slot holds is of a certain kind: a hand with a melee
	 * weapon in it can attack, whatever weapon it happens to be.
	 *
	 * Being a weapon is already written on the item as a fragment, so making every weapon repeat that it
	 * grants attacking would be saying the same thing twice, and forgetting to say it in a new one is a
	 * silent bug rather than a compile error.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot", meta = (Categories = "Capability"))
	TMap<TSubclassOf<UKzItemFragment>, FGameplayTagContainer> FragmentCapabilities;

	FKzEquipmentSlotDefinition()
		: DefaultSocketName(NAME_None)
	{
	}
};

/**
 * A Data Asset that defines the entire equipment layout (all available slots) for a specific type of entity.
 * Supports inheritance to avoid duplicating common slots across different layouts.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UKzEquipmentLayout : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Optional parent layout to inherit slots from.
	 * Useful for creating a base layout (e.g., Human) and extending it (e.g., HumanWithWings).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TObjectPtr<UKzEquipmentLayout> ParentLayout;

	/**
	 * Maps a logical slot alias to a physical slot ID.
	 * Example: Key = "Equipment.Slot.Hand.Main", Value = "Equipment.Slot.Hand.Right"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TMap<FGameplayTag, FGameplayTag> SlotAliases;

	/** The list of slots explicitly defined in this layout. Can override parent slots if they share the same SlotID. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (TitleProperty = "DisplayName"))
	TArray<FKzEquipmentSlotDefinition> Slots;

	/**
	 * Resolves a given Slot ID through the alias map.
	 * If the input is an alias (e.g., MainHand), returns the underlying physical slot ID (e.g., RightHand).
	 * Respects inheritance and overrides.
	 */
	FGameplayTag ResolveSlotID(const FGameplayTag& InSlotID) const;

	/** Helper to find a specific slot definition by its ID. Respects inheritance and local overrides. */
	bool FindSlotDefinition(const FGameplayTag& InSlotID, FKzEquipmentSlotDefinition& OutDefinition) const;

	/**
	 * Returns the socket name mapped to the given slot ID, resolving inheritance and overrides.
	 * Returns NAME_None if the slot is not found.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FName GetSocketForSlot(const FGameplayTag& InSlotID) const;

	/**
	 * Helper to get a flattened list of all slot definitions (Parent + Child), resolving any overrides.
	 * Very useful for initializing UI elements or the Equipment Component.
	 */
	void GetAllSlotDefinitions(TArray<FKzEquipmentSlotDefinition>& OutAllSlots) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITOR
	/** Checks if assigning the given parent would create a circular dependency loop. */
	bool HasCircularDependency(const UKzEquipmentLayout* PotentialParent) const;
#endif
};