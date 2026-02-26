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

	// Setup the grid with an optimal cell size for interaction (e.g., 2-3 meters)
	SpatialGrid.SetCellSize(GridCellSize);
}

void UInteractionSubsystem::Deinitialize()
{
	// Clean up the grid when the world is destroyed
	SpatialGrid.Reset();
	Super::Deinitialize();
}

void UInteractionSubsystem::RegisterInteractable(UInteractableComponent* Component)
{
	if (!Component) return;

	SpatialGrid.Insert(Component);
}

void UInteractionSubsystem::UnregisterInteractable(UInteractableComponent* Component)
{
	if (!Component) return;

	// Use the component's bounds to locate and remove it from the corresponding cells
	SpatialGrid.Remove(Component, Component->Bounds.GetBox());
}

void UInteractionSubsystem::UpdateInteractable(UInteractableComponent* Component, const FBox& OldBounds)
{
	if (!Component) return;

	SpatialGrid.Remove(Component, OldBounds);
	SpatialGrid.Insert(Component);
}

TArray<UInteractableComponent*> UInteractionSubsystem::QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const
{
	TArray<UInteractableComponent*> Results;
	SpatialGrid.Query(Results, QueryShape, ShapePosition, ShapeRotation);
	return Results;
}