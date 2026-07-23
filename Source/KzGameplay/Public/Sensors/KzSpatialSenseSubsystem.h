// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Spatial/KzSpatialRegistry.h"
#include "Sensors/KzSensableComponent.h"
#include "GameplayTagContainer.h"
#include "KzSpatialSenseSubsystem.generated.h"

/**
 * Semantics mapping for the Spatial Hash Grid to understand UKzSensableComponent.
 */
struct FKzSensableGridSemantics
{
	using ElementIdType = UKzSensableComponent*;

	static FBox GetBoundingBox(const UKzSensableComponent* E);
	static UKzSensableComponent* GetElementId(const UKzSensableComponent* E);
	static bool IsValid(const UKzSensableComponent* E);
	static FVector GetElementPosition(const UKzSensableComponent* E);
	static FKzShapeInstance GetShape(const UKzSensableComponent* E);
	static FQuat GetElementRotation(const UKzSensableComponent* E);
	static bool IsDynamic(const UKzSensableComponent* E);
};

/**
 * World Subsystem that manages all Sensable objects in the current world.
 * Uses a dual Spatial Hash Grid approach (Static + Dynamic) combined with native Gameplay Tag filtering.
 */
UCLASS()
class KZGAMEPLAY_API UKzSpatialSenseSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:
	/** Cell size used for the grid. Can be tuned based on typical sensor ranges. */
	float GridCellSize = 500.0f;

	/** Dual static/dynamic spatial registry of all sensables. */
	Kz::TSpatialRegistry<UKzSensableComponent*, FKzSensableGridSemantics> Registry;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//~ Begin UTickableWorldSubsystem Interface
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	//~ End UTickableWorldSubsystem Interface

	/**
	 * Registers a sensable component into the spatial grid.
	 * Typically called by the component's BeginPlay.
	 */
	void RegisterSensable(UKzSensableComponent* Component);

	/**
	 * Unregisters a sensable component from the spatial grid.
	 * Typically called by the component's EndPlay.
	 */
	void UnregisterSensable(UKzSensableComponent* Component);

	/**
	 * Performs a spatial query to find all sensables overlapping the given shape.
	 * @param QueryShape The volumetric shape to test against (e.g. Sensor's shape).
	 * @param ShapePosition World location of the query shape.
	 * @param ShapeRotation World rotation of the query shape.
	 * @param TagQuery Optional tag query to filter the results (e.g. "Creature AND NOT Dead").
	 * @return A list of candidate sensables.
	 */
	TArray<UKzSensableComponent*> QuerySensables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation, const FGameplayTagQuery& TagQuery) const;

	/** Returns all registered sensables in the world. */
	const TSet<UKzSensableComponent*>& GetAllRegisteredSensables() const
	{
		return Registry.GetRegistered();
	}
};