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

bool FKzSensableGridSemantics::IsDynamic(const UKzSensableComponent* E)
{
	return E && E->bIsDynamic;
}

// =================================================================
// SUBSYSTEM IMPLEMENTATION
// =================================================================

void UKzSpatialSenseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Registry.SetCellSize(GridCellSize);
}

void UKzSpatialSenseSubsystem::Deinitialize()
{
	Registry.Reset();
	Super::Deinitialize();
}

TStatId UKzSpatialSenseSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzSpatialSenseSubsystem, STATGROUP_Tickables);
}

void UKzSpatialSenseSubsystem::RegisterSensable(UKzSensableComponent* Component)
{
	Registry.Register(Component);
}

void UKzSpatialSenseSubsystem::UnregisterSensable(UKzSensableComponent* Component)
{
	Registry.Unregister(Component);
}

TArray<UKzSensableComponent*> UKzSpatialSenseSubsystem::QuerySensables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation, const FGameplayTagQuery& TagQuery) const
{
	TArray<UKzSensableComponent*> RawResults;
	Registry.Query(RawResults, QueryShape, ShapePosition, ShapeRotation);

	// Filter natively by Tags
	if (TagQuery.IsEmpty())
	{
		return RawResults;
	}

	TArray<UKzSensableComponent*> FilteredResults;
	FilteredResults.Reserve(RawResults.Num());

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
	Registry.TickDynamics();
}