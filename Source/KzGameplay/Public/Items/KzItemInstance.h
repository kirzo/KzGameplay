// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "KzItemInstance.generated.h"

class UKzItemDefinition;
class AActor;
class UMeshComponent;

/**
 * A single statistic entry for an item instance.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FItemStatEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** The tag identifying this stat (e.g., Stat.Ammo, Stat.Durability). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Stat")
	FGameplayTag StatTag;

	/** The current value of this stat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Stat")
	float Value = 0.0f;

	FItemStatEntry() = default;
	FItemStatEntry(FGameplayTag InStatTag, float InValue) : StatTag(InStatTag), Value(InValue) {}

	/** Checks if the value is functionally zero. */
	bool IsZero() const
	{
		return FMath::IsNearlyZero(Value);
	}

	/** Equality operator against another entry. */
	bool operator==(const FItemStatEntry& Other) const
	{
		return StatTag == Other.StatTag && FMath::IsNearlyEqual(Value, Other.Value);
	}

	bool operator!=(const FItemStatEntry& Other) const
	{
		return !(*this == Other);
	}

	/** Equality operator against a GameplayTag (Allows using TArray::FindByKey). */
	bool operator==(FGameplayTag OtherTag) const
	{
		return StatTag == OtherTag;
	}
};

/**
 * Container for item stats that handles efficient network replication.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FItemStatContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Stats")
	TArray<FItemStatEntry> Items;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FItemStatEntry, FItemStatContainer>(Items, DeltaParms, *this);
	}

	/** Returns the value of a stat, or DefaultValue if not found. */
	float GetStat(FGameplayTag StatTag, float DefaultValue = 0.0f) const
	{
		if (const FItemStatEntry* FoundEntry = Items.FindByKey(StatTag))
		{
			return FoundEntry->Value;
		}
		return DefaultValue;
	}

	/** Adds or updates a stat, marking it for replication if changed. */
	void SetStat(FGameplayTag StatTag, float Value)
	{
		if (FItemStatEntry* FoundEntry = Items.FindByKey(StatTag))
		{
			// Only update and mark dirty if the value actually changed
			if (!FMath::IsNearlyEqual(FoundEntry->Value, Value))
			{
				FoundEntry->Value = Value;
				MarkItemDirty(*FoundEntry);
			}
		}
		else
		{
			// Stat not found, add a new one and mark it dirty
			FItemStatEntry& NewEntry = Items.Add_GetRef(FItemStatEntry(StatTag, Value));
			MarkItemDirty(NewEntry);
		}
	}
};

/** Boilerplate required to tell the engine this struct has a custom net serializer. */
template<>
struct TStructOpsTypeTraits<FItemStatContainer> : public TStructOpsTypeTraitsBase2<FItemStatContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * Represents a live instance of an item in an inventory or equipment slot.
 * Bridges the static data (Definition) with the runtime state (Quantity, Physical Actor, Stats).
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

	/** Dynamic statistics attached to this specific item instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance")
	FItemStatContainer Stats;

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