// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzItemInstance.generated.h"

class UKzItemDefinition;
class AActor;
class UMeshComponent;

/**
 * Represents a live instance of an item in an inventory or equipment slot.
 * Bridges the static data (Definition) with the runtime state (Quantity, Physical Actor).
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzItemInstance
{
	GENERATED_BODY()

public:
	/** The immutable definition and rules of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance")
	TObjectPtr<const UKzItemDefinition> ItemDef = nullptr;

	/** The current stack quantity of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance", meta = (ClampMin = "1"))
	int32 Quantity = 0;

	/** The actor representing this item in the world or equipped (Valid if SpawnMode is SpawnActor). */
	UPROPERTY(BlueprintReadOnly, Category = "Item Instance")
	TObjectPtr<AActor> SpawnedActor = nullptr;

	/** The mesh component representing this item when equipped (Valid if SpawnMode is SpawnMesh). */
	UPROPERTY(BlueprintReadOnly, Category = "Item Instance")
	TObjectPtr<UMeshComponent> SpawnedComponent = nullptr;

	/** Runtime instance of the Acquired action. Holds state while in the inventory. */
	UPROPERTY()
	FScriptableAction ActiveAcquiredAction;

	/** Runtime instance of the Equip action. Holds state while in the equipment. */
	UPROPERTY()
	FScriptableAction ActiveEquippedAction;

	FKzItemInstance() = default;

	FKzItemInstance(const UKzItemDefinition* InDef, int32 InQuantity, AActor* InSpawnedActor = nullptr)
		: ItemDef(InDef)
		, Quantity(InQuantity)
		, SpawnedActor(InSpawnedActor)
	{
	}

	/** Checks if this instance contains a valid item definition. */
	bool IsValid() const
	{
		return ItemDef != nullptr && Quantity > 0;
	}

	/** Equality operator to easily find specific instances in an array. */
	bool operator==(const FKzItemInstance& Other) const
	{
		return ItemDef == Other.ItemDef && SpawnedActor == Other.SpawnedActor;
	}

	/** Checks if this instance currently has any physical representation in the world. */
	bool HasPhysicalRepresentation() const
	{
		return SpawnedActor != nullptr || SpawnedComponent != nullptr;
	}
};