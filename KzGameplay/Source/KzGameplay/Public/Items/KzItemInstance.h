// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "KzItemInstance.generated.h"

class UKzItemDefinition;
class UKzInventoryComponent;
class AActor;
class UMeshComponent;

/**
 * A single statistic entry for an item instance.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FItemStatEntry
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

	bool operator==(const FItemStatEntry& Other) const
	{
		return StatTag == Other.StatTag && FMath::IsNearlyEqual(Value, Other.Value);
	}

	bool operator!=(const FItemStatEntry& Other) const
	{
		return !(*this == Other);
	}

	/** Equality against a GameplayTag (allows using TArray::FindByKey). */
	bool operator==(FGameplayTag OtherTag) const
	{
		return StatTag == OtherTag;
	}
};

/**
 * Container for an item instance's dynamic stats.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FItemStatContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Stats")
	TArray<FItemStatEntry> Items;

	/** Returns the value of a stat, or DefaultValue if not found. */
	float GetStat(FGameplayTag StatTag, float DefaultValue = 0.0f) const
	{
		if (const FItemStatEntry* FoundEntry = Items.FindByKey(StatTag))
		{
			return FoundEntry->Value;
		}
		return DefaultValue;
	}

	/** Adds or updates a stat. */
	void SetStat(FGameplayTag StatTag, float Value)
	{
		if (FItemStatEntry* FoundEntry = Items.FindByKey(StatTag))
		{
			FoundEntry->Value = Value;
		}
		else
		{
			Items.Add(FItemStatEntry(StatTag, Value));
		}
	}
};

struct FKzInventoryList;

/**
 * Represents a live instance of an item in an inventory or equipment slot.
 * Bridges the static data (Definition) with the runtime state (Quantity, Physical Actor, Stats).
 * A FastArray item, so inventories replicate per-entry deltas.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzItemInstance : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	/** The immutable definition and rules of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance")
	TObjectPtr<const UKzItemDefinition> ItemDef = nullptr;

	/** The current stack quantity of this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Dynamic statistics attached to this specific item instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Instance")
	FItemStatContainer Stats;

	/** The actor representing this item in the world or equipped (Valid if SpawnMode is SpawnActor). */
	UPROPERTY(BlueprintReadOnly, Category = "Item Instance")
	TObjectPtr<AActor> SpawnedActor = nullptr;

	/** Local cosmetic mesh when equipped (SpawnMesh mode). Not replicated: each machine spawns its own. */
	UPROPERTY(BlueprintReadOnly, NotReplicated, Transient, Category = "Item Instance")
	TObjectPtr<UMeshComponent> SpawnedComponent = nullptr;

	/** Runtime instance of the Acquired action. Holds state while in the inventory. Server-only. */
	UPROPERTY(NotReplicated)
	FScriptableAction ActiveAcquiredAction;

	/** Runtime instance of the Equip action. Holds state while in the equipment. Server-only. */
	UPROPERTY(NotReplicated)
	FScriptableAction ActiveEquippedAction;

	FKzItemInstance() = default;
	FKzItemInstance(const UKzItemDefinition* InDef, int32 InQuantity = 1, AActor* InSpawnedActor = nullptr);

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

	/** Initializes a newly created item instance by running all fragment initialization logic. */
	void Initialize(const UKzItemDefinition* ItemDefinition);

	// FastArray client-side callbacks: notify the owning component so UI can refresh.
	void PostReplicatedAdd(const FKzInventoryList& InArraySerializer);
	void PostReplicatedChange(const FKzInventoryList& InArraySerializer);
	void PreReplicatedRemove(const FKzInventoryList& InArraySerializer);
};

/**
 * FastArray of item instances, giving delta replication and per-item client callbacks.
 */
USTRUCT()
struct KZGAMEPLAY_API FKzInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	/** The replicated item entries. */
	UPROPERTY()
	TArray<FKzItemInstance> Items;

	/** Owning component, used by item callbacks to broadcast changes. Set on construction; not replicated. */
	UPROPERTY(NotReplicated)
	TObjectPtr<UKzInventoryComponent> OwnerComponent = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FKzItemInstance, FKzInventoryList>(Items, DeltaParms, *this);
	}
};

/** Boilerplate required to tell the engine this struct has a custom net delta serializer. */
template<>
struct TStructOpsTypeTraits<FKzInventoryList> : public TStructOpsTypeTraitsBase2<FKzInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};