// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
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

	UE_DISABLE_OPTIMIZATION

	/**
	 * Resolves a given Slot ID through the alias map.
	 * If the input is an alias (e.g., MainHand), returns the underlying physical slot ID (e.g., RightHand).
	 * Respects inheritance and overrides.
	 */
	FGameplayTag ResolveSlotID(const FGameplayTag& InSlotID) const
	{
		// 1. Check local aliases first (allows a child to change MainHand from Right to Left)
		if (const FGameplayTag* TargetTag = SlotAliases.Find(InSlotID))
		{
			return *TargetTag;
		}

		// 2. If not found locally, check if the parent layout knows this alias
		if (ParentLayout && ParentLayout != this)
		{
			return ParentLayout->ResolveSlotID(InSlotID);
		}

		// 3. If it's not an alias anywhere, assume it's already a physical Slot ID
		return InSlotID;
	}

	UE_ENABLE_OPTIMIZATION

	/** Helper to find a specific slot definition by its ID. Respects inheritance and local overrides. */
	bool FindSlotDefinition(const FGameplayTag& InSlotID, FKzEquipmentSlotDefinition& OutDefinition) const
	{
		// First, resolve the alias (MainHand -> RightHand)
		const FGameplayTag ResolvedID = ResolveSlotID(InSlotID);

		// 1. Check local slots first (this allows child layouts to override parent socket names or display names)
		for (const FKzEquipmentSlotDefinition& SlotDef : Slots)
		{
			if (SlotDef.SlotID == ResolvedID)
			{
				OutDefinition = SlotDef;
				return true;
			}
		}

		// 2. If not found locally, delegate to the parent layout
		if (ParentLayout && ParentLayout != this)
		{
			return ParentLayout->FindSlotDefinition(ResolvedID, OutDefinition);
		}

		return false;
	}

	/**
	 * Returns the socket name mapped to the given slot ID, resolving inheritance and overrides.
	 * Returns NAME_None if the slot is not found.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FName GetSocketForSlot(const FGameplayTag& InSlotID) const
	{
		FKzEquipmentSlotDefinition FoundDef;
		if (FindSlotDefinition(InSlotID, FoundDef))
		{
			return FoundDef.DefaultSocketName;
		}

		return NAME_None;
	}

	/**
	 * Helper to get a flattened list of all slot definitions (Parent + Child), resolving any overrides.
	 * Very useful for initializing UI elements or the Equipment Component.
	 */
	void GetAllSlotDefinitions(TArray<FKzEquipmentSlotDefinition>& OutAllSlots) const
	{
		// Start with parent slots if available
		if (ParentLayout && ParentLayout != this)
		{
			ParentLayout->GetAllSlotDefinitions(OutAllSlots);
		}

		// Add or override with local slots
		for (const FKzEquipmentSlotDefinition& LocalSlot : Slots)
		{
			// Try to find if this slot already exists from the parent
			int32 ExistingIndex = OutAllSlots.IndexOfByPredicate([&LocalSlot](const FKzEquipmentSlotDefinition& S)
				{
					return S.SlotID == LocalSlot.SlotID;
				});

			if (ExistingIndex != INDEX_NONE)
			{
				// Override parent's definition
				OutAllSlots[ExistingIndex] = LocalSlot;
			}
			else
			{
				// Add as a new slot
				OutAllSlots.Add(LocalSlot);
			}
		}
	}
};