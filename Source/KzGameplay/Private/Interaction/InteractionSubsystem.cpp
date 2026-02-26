// Copyright 2026 kirzo

#include "Interaction/InteractionSubsystem.h"
#include "Components/KzShapeComponent.h"

// =================================================================
// SEMANTICS IMPLEMENTATION
// =================================================================

FBox FInteractionGridSemantics::GetBoundingBox(const UInteractableComponent* E)
{
	return E ? E->Shape.GetBoundingBox(E->GetComponentTransform()) : FBox(EForceInit::ForceInit);
}

UInteractableComponent* FInteractionGridSemantics::GetElementId(const UInteractableComponent* E)
{
	return const_cast<UInteractableComponent*>(E);
}

bool FInteractionGridSemantics::IsValid(const UInteractableComponent* E)
{
	return ::IsValid(E);
}

FVector FInteractionGridSemantics::GetElementPosition(const UInteractableComponent* E)
{
	return E ? E->GetComponentLocation() : FVector::ZeroVector;
}

FKzShapeInstance FInteractionGridSemantics::GetShape(const UInteractableComponent* E)
{
	return E ? E->Shape : FKzShapeInstance();
}

FQuat FInteractionGridSemantics::GetElementRotation(const UInteractableComponent* E)
{
	return E ? E->GetComponentQuat() : FQuat::Identity;
}

// =================================================================
// SUBSYSTEM IMPLEMENTATION
// =================================================================

void UInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	StaticGrid.SetCellSize(GridCellSize);
	DynamicGrid.SetCellSize(GridCellSize);
}

void UInteractionSubsystem::Deinitialize()
{
	// Clean up the grid when the world is destroyed
	StaticGrid.Reset();
	DynamicGrid.Reset();
	Super::Deinitialize();
}

TStatId UInteractionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UInteractionSubsystem, STATGROUP_Tickables);
}

void UInteractionSubsystem::RegisterInteractable(UInteractableComponent* Component)
{
	if (!Component || RegisteredComponents.Contains(Component))
	{
		return;
	}

	// Mark as registered
	RegisteredComponents.Add(Component);

	if (Component->bIsDynamicInteraction)
	{
		DynamicInteractables.Add(Component);
		DynamicGrid.Insert(Component);
	}
	else
	{
		StaticGrid.Insert(Component);
	}
}

void UInteractionSubsystem::UnregisterInteractable(UInteractableComponent* Component)
{
	if (!Component || !RegisteredComponents.Contains(Component))
	{
		return;
	}

	// Remove from the master tracker
	RegisteredComponents.Remove(Component);

	if (Component->bIsDynamicInteraction)
	{
		// Remove from tracking array
		int32 Index = DynamicInteractables.IndexOfByKey(Component);
		if (Index != INDEX_NONE)
		{
			// O(1) removal using the cached bounds
			DynamicGrid.Remove(Component, DynamicInteractables[Index].LastBounds);
			DynamicInteractables.RemoveAtSwap(Index);
		}
	}
	else
	{
		// Static objects use their current bounds because they never changed
		StaticGrid.Remove(Component, Component->Bounds.GetBox());
	}
}

void UInteractionSubsystem::UpdateInteractable(UInteractableComponent* Component, const FBox& OldBounds)
{
	if (!Component || Component->bIsDynamicInteraction) return;

	StaticGrid.Remove(Component, OldBounds);
	StaticGrid.Insert(Component);
}

TArray<UInteractableComponent*> UInteractionSubsystem::QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const
{
	TArray<UInteractableComponent*> Results;
	StaticGrid.Query(Results, QueryShape, ShapePosition, ShapeRotation);
	DynamicGrid.Query(Results, QueryShape, ShapePosition, ShapeRotation);
	return Results;
}

void UInteractionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fast iteration over a dense contiguous array
	for (int32 i = DynamicInteractables.Num() - 1; i >= 0; --i)
	{
		FDynamicInteractableTrack& Track = DynamicInteractables[i];

		// Safety check in case the component was destroyed without calling Unregister
		if (!FInteractionGridSemantics::IsValid(Track.Component))
		{
			DynamicInteractables.RemoveAtSwap(i);
			continue;
		}

		// Calculate the new bounding box using unified semantics
		const FBox CurrentBounds = FInteractionGridSemantics::GetBoundingBox(Track.Component);

		// Check if the component's bounds have changed (movement, rotation, or scale/shape changes)
		if (!CurrentBounds.Equals(Track.LastBounds))
		{
			DynamicGrid.Remove(Track.Component, Track.LastBounds);
			Track.LastBounds = CurrentBounds;
			DynamicGrid.Insert(Track.Component);
		}
	}
}