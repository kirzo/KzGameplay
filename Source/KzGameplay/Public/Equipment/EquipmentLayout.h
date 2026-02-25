// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EquipmentLayout.generated.h"

/**
 * Defines a single equipment slot available for a character/entity.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FEquipmentSlotDefinition
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

	FEquipmentSlotDefinition()
		: DefaultSocketName(NAME_None)
	{
	}
};

/**
 * A Data Asset that defines the entire equipment layout (all available slots) for a specific type of entity.
 * Supports inheritance to avoid duplicating common slots across different layouts.
 */
UCLASS(BlueprintType, Const)
class KZGAMEPLAY_API UEquipmentLayout : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Optional parent layout to inherit slots from.
	 * Useful for creating a base layout (e.g., Human) and extending it (e.g., HumanWithWings).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TObjectPtr<UEquipmentLayout> ParentLayout;

	/** The list of slots explicitly defined in this layout. Can override parent slots if they share the same SlotID. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (TitleProperty = "DisplayName"))
	TArray<FEquipmentSlotDefinition> Slots;

	/** Helper to find a specific slot definition by its ID. Respects inheritance and local overrides. */
	bool FindSlotDefinition(const FGameplayTag& InSlotID, FEquipmentSlotDefinition& OutDefinition) const
	{
		// 1. Check local slots first (this allows child layouts to override parent socket names or display names)
		for (const FEquipmentSlotDefinition& SlotDef : Slots)
		{
			if (SlotDef.SlotID == InSlotID)
			{
				OutDefinition = SlotDef;
				return true;
			}
		}

		// 2. If not found locally, delegate to the parent layout
		if (ParentLayout && ParentLayout != this)
		{
			return ParentLayout->FindSlotDefinition(InSlotID, OutDefinition);
		}

		return false;
	}

	/**
	 * Helper to get a flattened list of all slot definitions (Parent + Child), resolving any overrides.
	 * Very useful for initializing UI elements or the Equipment Component.
	 */
	void GetAllSlotDefinitions(TArray<FEquipmentSlotDefinition>& OutAllSlots) const
	{
		// Start with parent slots if available
		if (ParentLayout && ParentLayout != this)
		{
			ParentLayout->GetAllSlotDefinitions(OutAllSlots);
		}

		// Add or override with local slots
		for (const FEquipmentSlotDefinition& LocalSlot : Slots)
		{
			// Try to find if this slot already exists from the parent
			int32 ExistingIndex = OutAllSlots.IndexOfByPredicate([&LocalSlot](const FEquipmentSlotDefinition& S)
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