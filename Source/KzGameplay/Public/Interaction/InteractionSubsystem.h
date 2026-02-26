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

/** Tracks the state of a dynamic interactable to detect movements and update the grid efficiently. */
struct FDynamicInteractableTrack
{
	UInteractableComponent* Component = nullptr;
	FBox LastBounds = FBox(EForceInit::ForceInit);

	FDynamicInteractableTrack() {}
	FDynamicInteractableTrack(UInteractableComponent* InComp)
		: Component(InComp)
	{
		LastBounds = FInteractionGridSemantics::GetBoundingBox(InComp);
	}

	bool operator==(const FDynamicInteractableTrack& Other) const
	{
		return Component == Other.Component;
	}

	bool operator!=(const FDynamicInteractableTrack& Other) const
	{
		return Component != Other.Component;
	}
};

/**
 * World Subsystem that manages all interactable objects in the current world.
 * Uses a dual Spatial Hash Grid approach (Static + Dynamic) for maximum performance.
 */
UCLASS()
class KZGAMEPLAY_API UInteractionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:
	/** Cell size used for the grid. Can be tuned based on typical interaction ranges. */
	float GridCellSize = 200.0f;

	/** Grid for objects that never move (Doors, Chests, Plants). Zero CPU cost per frame. */
	Kz::TSpatialHashGrid<UInteractableComponent*, FInteractionGridSemantics> StaticGrid;

	/** Grid for objects that move (NPCs, Physics Items). */
	Kz::TSpatialHashGrid<UInteractableComponent*, FInteractionGridSemantics> DynamicGrid;

	/** Fast O(1) lookup to prevent duplicate registrations and manage state safely. */
	UPROPERTY(Transient)
	TSet<TObjectPtr<UInteractableComponent>> RegisteredComponents;

	/** Dense array tracking dynamic elements to update them automatically when they move. */
	TArray<FDynamicInteractableTrack> DynamicInteractables;

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