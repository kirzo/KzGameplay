// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Spatial/KzSpatialRegistry.h"
#include "Interaction/KzInteractableComponent.h"
#include "KzInteractionSubsystem.generated.h"

struct FInteractionGridSemantics
{
	// Using the raw pointer as the ID since we don't need dense handle storage here
	using ElementIdType = UKzInteractableComponent*;

	static FBox GetBoundingBox(const UKzInteractableComponent* E);
	static UKzInteractableComponent* GetElementId(const UKzInteractableComponent* E);
	static bool IsValid(const UKzInteractableComponent* E);
	static FVector GetElementPosition(const UKzInteractableComponent* E);
	static FKzShapeInstance GetShape(const UKzInteractableComponent* E);
	static FQuat GetElementRotation(const UKzInteractableComponent* E);
	static bool IsDynamic(const UKzInteractableComponent* E);
};

/**
 * World Subsystem that manages all interactable objects in the current world.
 * Uses a dual Spatial Hash Grid approach (Static + Dynamic) for maximum performance.
 */
UCLASS()
class KZGAMEPLAY_API UKzInteractionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:
	/** Cell size used for the grid. Can be tuned based on typical interaction ranges. */
	float GridCellSize = 200.0f;

	/** Dual static/dynamic spatial registry of all interactables. */
	Kz::TSpatialRegistry<UKzInteractableComponent*, FInteractionGridSemantics> Registry;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// UTickableWorldSubsystem interface
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Registers an interactable component into the spatial grid.
	 * Typically called by the component's BeginPlay.
	 */
	void RegisterInteractable(UKzInteractableComponent* Component);

	/**
	 * Unregisters an interactable component from the spatial grid.
	 * Typically called by the component's EndPlay.
	 */
	void UnregisterInteractable(UKzInteractableComponent* Component);

	/**
	 * Performs a spatial query to find all interactables overlapping the given shape.
	 * @param QueryShape The volumetric shape to test against.
	 * @param ShapePosition World location of the query shape.
	 * @param ShapeRotation World rotation of the query shape.
	 * @return A list of candidate interactables.
	 */
	TArray<UKzInteractableComponent*> QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const;

	/** Returns all registered interactables in the world. */
	const TSet<UKzInteractableComponent*>& GetAllRegisteredInteractables() const
	{
		return Registry.GetRegistered();
	}
};