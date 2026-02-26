// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Spatial/KzSpatialHashGrid.h"
#include "Interaction/InteractableComponent.h"
#include "InteractionSubsystem.generated.h"

struct FInteractionGridSemantics
{
	// Using the raw pointer as the ID since we don't need dense handle storage here
	using ElementIdType = UInteractableComponent*;

	static FBox GetBoundingBox(const UInteractableComponent* E);
	static UInteractableComponent* GetElementId(const UInteractableComponent* E);
	static bool IsValid(const UInteractableComponent* E);
	static FVector GetElementPosition(const UInteractableComponent* E);
	static FKzShapeInstance GetShape(const UInteractableComponent* E);
	static FQuat GetElementRotation(const UInteractableComponent* E);
};

/**
 * World Subsystem that manages all interactable objects in the current world.
 * Uses a Spatial Hash Grid to provide fast proximity queries for Interactor Components.
 */
UCLASS()
class KZGAMEPLAY_API UInteractionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

private:
	/** Spatial Hash Grid for broad-phase interaction queries. */
	Kz::TSpatialHashGrid<UInteractableComponent*, FInteractionGridSemantics> SpatialGrid;

	/** Cell size used for the grid. Can be tuned based on typical interaction ranges. */
	float GridCellSize = 200.0f;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Registers an interactable component into the spatial grid.
	 * Typically called by the component's BeginPlay.
	 */
	void RegisterInteractable(UInteractableComponent* Component);

	/**
	 * Unregisters an interactable component from the spatial grid.
	 * Typically called by the component's EndPlay.
	 */
	void UnregisterInteractable(UInteractableComponent* Component);

	/**
	 * Updates a component's position in the grid.
	 * @param Component The component that moved.
	 * @param OldBounds The previous bounding box before the movement, needed to clear old cells.
	 */
	void UpdateInteractable(UInteractableComponent* Component, const FBox& OldBounds);

	/**
	 * Performs a spatial query to find all interactables overlapping the given shape.
	 * @param QueryShape The volumetric shape to test against.
	 * @param ShapePosition World location of the query shape.
	 * @param ShapeRotation World rotation of the query shape.
	 * @return A list of candidate interactables.
	 */
	TArray<UInteractableComponent*> QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const;
};