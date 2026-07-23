// Copyright 2026 kirzo

#include "Interaction/KzInteractionSubsystem.h"
#include "Components/KzShapeComponent.h"

// =================================================================
// SEMANTICS IMPLEMENTATION
// =================================================================

FBox FInteractionGridSemantics::GetBoundingBox(const UKzInteractableComponent* E)
{
	return E ? E->Shape.GetBoundingBox(E->GetComponentTransform()) : FBox(EForceInit::ForceInit);
}

UKzInteractableComponent* FInteractionGridSemantics::GetElementId(const UKzInteractableComponent* E)
{
	return const_cast<UKzInteractableComponent*>(E);
}

bool FInteractionGridSemantics::IsValid(const UKzInteractableComponent* E)
{
	return ::IsValid(E);
}

FVector FInteractionGridSemantics::GetElementPosition(const UKzInteractableComponent* E)
{
	return E ? E->GetComponentLocation() : FVector::ZeroVector;
}

FKzShapeInstance FInteractionGridSemantics::GetShape(const UKzInteractableComponent* E)
{
	return E ? E->Shape : FKzShapeInstance();
}

FQuat FInteractionGridSemantics::GetElementRotation(const UKzInteractableComponent* E)
{
	return E ? E->GetComponentQuat() : FQuat::Identity;
}

bool FInteractionGridSemantics::IsDynamic(const UKzInteractableComponent* E)
{
	return E && E->bIsDynamicInteraction;
}

// =================================================================
// SUBSYSTEM IMPLEMENTATION
// =================================================================

void UKzInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Registry.SetCellSize(GridCellSize);
}

void UKzInteractionSubsystem::Deinitialize()
{
	Registry.Reset();
	Super::Deinitialize();
}

TStatId UKzInteractionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzInteractionSubsystem, STATGROUP_Tickables);
}

void UKzInteractionSubsystem::RegisterInteractable(UKzInteractableComponent* Component)
{
	Registry.Register(Component);
}

void UKzInteractionSubsystem::UnregisterInteractable(UKzInteractableComponent* Component)
{
	Registry.Unregister(Component);
}

TArray<UKzInteractableComponent*> UKzInteractionSubsystem::QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const
{
	TArray<UKzInteractableComponent*> Results;
	Registry.Query(Results, QueryShape, ShapePosition, ShapeRotation);
	return Results;
}

void UKzInteractionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Registry.TickDynamics();
}