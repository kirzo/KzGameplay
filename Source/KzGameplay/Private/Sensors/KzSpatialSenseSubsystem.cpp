// Copyright 2026 kirzo

#include "Sensors/KzSpatialSenseSubsystem.h"
#include "GameFramework/Actor.h"

// =================================================================
// SEMANTICS IMPLEMENTATION
// =================================================================

FBox FKzSensableGridSemantics::GetBoundingBox(const UKzSensableComponent* E)
{
	return E ? E->GetBounds() : FBox(EForceInit::ForceInit);
}

UKzSensableComponent* FKzSensableGridSemantics::GetElementId(const UKzSensableComponent* E)
{
	return const_cast<UKzSensableComponent*>(E);
}

bool FKzSensableGridSemantics::IsValid(const UKzSensableComponent* E)
{
	return ::IsValid(E);
}

FVector FKzSensableGridSemantics::GetElementPosition(const UKzSensableComponent* E)
{
	return E ? E->GetShapeLocation() : FVector::ZeroVector;
}

FKzShapeInstance FKzSensableGridSemantics::GetShape(const UKzSensableComponent* E)
{
	return E ? E->GetShapeInstance() : FKzShapeInstance();
}

FQuat FKzSensableGridSemantics::GetElementRotation(const UKzSensableComponent* E)
{
	return E ? E->GetShapeRotation() : FQuat::Identity;
}

// =================================================================
// SUBSYSTEM IMPLEMENTATION
// =================================================================

void UKzSpatialSenseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	StaticGrid.SetCellSize(GridCellSize);
	DynamicGrid.SetCellSize(GridCellSize);
}

void UKzSpatialSenseSubsystem::Deinitialize()
{
	StaticGrid.Reset();
	DynamicGrid.Reset();
	Super::Deinitialize();
}

TStatId UKzSpatialSenseSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzSpatialSenseSubsystem, STATGROUP_Tickables);
}

void UKzSpatialSenseSubsystem::RegisterSensable(UKzSensableComponent* Component)
{
	if (!Component || RegisteredComponents.Contains(Component))
	{
		return;
	}

	RegisteredComponents.Add(Component);

	if (Component->bIsDynamic)
	{
		DynamicSensables.Add(FDynamicSensableTrack(Component));
		DynamicGrid.Insert(Component);
	}
	else
	{
		StaticGrid.Insert(Component);
	}
}

void UKzSpatialSenseSubsystem::UnregisterSensable(UKzSensableComponent* Component)
{
	if (!Component || !RegisteredComponents.Contains(Component))
	{
		return;
	}

	RegisteredComponents.Remove(Component);

	if (Component->bIsDynamic)
	{
		int32 Index = DynamicSensables.IndexOfByKey(Component);
		if (Index != INDEX_NONE)
		{
			DynamicGrid.Remove(Component, DynamicSensables[Index].LastBounds);
			DynamicSensables.RemoveAtSwap(Index);
		}
	}
	else
	{
		StaticGrid.Remove(Component, Component->GetBounds());
	}
}

TArray<UKzSensableComponent*> UKzSpatialSenseSubsystem::QuerySensables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation, const FGameplayTagQuery& TagQuery) const
{
	TArray<UKzSensableComponent*> RawResults;

	// 1. Broad + Narrow phase via GJK in the Grids (O(1) Spatial lookup)
	StaticGrid.Query(RawResults, QueryShape, ShapePosition, ShapeRotation);
	DynamicGrid.Query(RawResults, QueryShape, ShapePosition, ShapeRotation);

	// 2. Filter natively by Tags
	if (TagQuery.IsEmpty())
	{
		return RawResults;
	}

	TArray<UKzSensableComponent*> FilteredResults;
	FilteredResults.Reserve(RawResults.Num()); // Optimize allocation

	for (UKzSensableComponent* Sensable : RawResults)
	{
		if (TagQuery.Matches(Sensable->SenseTags))
		{
			FilteredResults.Add(Sensable);
		}
	}

	return FilteredResults;
}

void UKzSpatialSenseSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fast iteration over a dense contiguous array (backward to allow safe removals)
	for (int32 i = DynamicSensables.Num() - 1; i >= 0; --i)
	{
		FDynamicSensableTrack& Track = DynamicSensables[i];

		// Safety check in case the component was destroyed/GC'd without calling Unregister
		if (!FKzSensableGridSemantics::IsValid(Track.Component))
		{
			DynamicSensables.RemoveAtSwap(i);
			continue;
		}

		// Calculate the new bounding box
		const FBox CurrentBounds = Track.Component->GetBounds();

		// Check if the component's bounds have changed (movement, rotation, or scale)
		if (!CurrentBounds.Equals(Track.LastBounds))
		{
			DynamicGrid.Remove(Track.Component, Track.LastBounds);
			Track.LastBounds = CurrentBounds;
			DynamicGrid.Insert(Track.Component);
		}
	}
}