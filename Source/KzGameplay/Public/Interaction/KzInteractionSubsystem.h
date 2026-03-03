// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Spatial/KzSpatialHashGrid.h"
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
};

/** Tracks the state of a dynamic interactable to detect movements and update the grid efficiently. */
struct FDynamicInteractableTrack
{
	UKzInteractableComponent* Component = nullptr;
	FBox LastBounds = FBox(EForceInit::ForceInit);

	FDynamicInteractableTrack() {}
	FDynamicInteractableTrack(UKzInteractableComponent* InComp)
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
class KZGAMEPLAY_API UKzInteractionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:
	/** Cell size used for the grid. Can be tuned based on typical interaction ranges. */
	float GridCellSize = 200.0f;

	/** Grid for objects that never move (Doors, Chests, Plants). Zero CPU cost per frame. */
	Kz::TSpatialHashGrid<UKzInteractableComponent*, FInteractionGridSemantics> StaticGrid;

	/** Grid for objects that move (NPCs, Physics Items). */
	Kz::TSpatialHashGrid<UKzInteractableComponent*, FInteractionGridSemantics> DynamicGrid;

	/** Fast O(1) lookup to prevent duplicate registrations and manage state safely. */
	UPROPERTY(Transient)
	TSet<TObjectPtr<UKzInteractableComponent>> RegisteredComponents;

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
	void RegisterInteractable(UKzInteractableComponent* Component);

	/**
	 * Unregisters an interactable component from the spatial grid.
	 * Typically called by the component's EndPlay.
	 */
	void UnregisterInteractable(UKzInteractableComponent* Component);

	/**
	 * Updates a component's position in the grid.
	 * @param Component The component that moved.
	 * @param OldBounds The previous bounding box before the movement, needed to clear old cells.
	 */
	void UpdateInteractable(UKzInteractableComponent* Component, const FBox& OldBounds);

	/**
	 * Performs a spatial query to find all interactables overlapping the given shape.
	 * @param QueryShape The volumetric shape to test against.
	 * @param ShapePosition World location of the query shape.
	 * @param ShapeRotation World rotation of the query shape.
	 * @return A list of candidate interactables.
	 */
	TArray<UKzInteractableComponent*> QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const;

	/** Returns all registered interactables in the world. */
	const TSet<TObjectPtr<UKzInteractableComponent>>& GetAllRegisteredInteractables() const
	{
		return RegisteredComponents;
	}
};