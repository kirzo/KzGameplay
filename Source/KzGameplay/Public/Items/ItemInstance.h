// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ItemInstance.generated.h"

class UItemDefinition;
class AActor;

/**
 * Represents a live instance of an item in an inventory or equipment slot.
 * Bridges the static data (Definition) with the runtime state (Quantity, Physical Actor).
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FItemInstance
{
	GENERATED_BODY()

public:
	/** The immutable definition and rules of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance")
	TObjectPtr<const UItemDefinition> ItemDef = nullptr;

	/** The current stack quantity of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance", meta = (ClampMin = "1"))
	int32 Quantity = 0;

	/** Pointer to the physical actor in the world. */
	UPROPERTY(BlueprintReadWrite, Category = "Item Instance")
	TObjectPtr<AActor> PhysicalActor = nullptr;

	FItemInstance() = default;

	FItemInstance(const UItemDefinition* InDef, int32 InQuantity, AActor* InPhysicalActor = nullptr)
		: ItemDef(InDef)
		, Quantity(InQuantity)
		, PhysicalActor(InPhysicalActor)
	{
	}

	/** Checks if this instance contains a valid item definition. */
	bool IsValid() const
	{
		return ItemDef != nullptr && Quantity > 0;
	}

	/** Equality operator to easily find specific instances in an array. */
	bool operator==(const FItemInstance& Other) const
	{
		return ItemDef == Other.ItemDef && PhysicalActor == Other.PhysicalActor;
	}
};