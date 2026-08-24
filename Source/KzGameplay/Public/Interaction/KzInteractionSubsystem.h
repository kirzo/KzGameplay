// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Spatial/KzSpatialRegistry.h"
#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractionTypes.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKzInteractionBegun, const FKzInteraction&, Interaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKzInteractionEnded, const FKzInteraction&, Interaction, EKzInteractionEndReason, Reason);

/**
 * World Subsystem that owns every live interaction and the spatial index used to find interactables.
 *
 * Interactions live here and nowhere else, so there is one truth about who is interacting with what and one
 * place where an interaction can end. Anyone may end one, and everyone hears it through OnInteractionEnded.
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

	/** Every live interaction, keyed by handle. Weak pointers throughout, so it needs no reflection. */
	TMap<FKzInteractionHandle, FKzInteraction> Interactions;

	/** Monotonic handle counter. Never reused, so stale handles read as inactive instead of aliasing. */
	int32 NextHandleId = 1;

	/** Seconds between keep-alive passes. */
	static constexpr float ValidationInterval = 0.1f;

	/** Time left before the next keep-alive pass. */
	float TimeUntilValidation = 0.0f;

	/** Guards against re-entrancy: ending an interaction can end others through listeners. */
	bool bValidating = false;

public:
	/** Fired when an interaction starts, after listeners can already look it up. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnKzInteractionBegun OnInteractionBegun;

	/** Fired when an interaction ends, whatever ended it. The interaction is already gone from the registry. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnKzInteractionEnded OnInteractionEnded;

	/** True where interactions are decided: the server, or anything not running as a client. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasInteractionAuthority() const;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// UTickableWorldSubsystem interface
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// INTERACTIONS
	// ==========================================

	/**
	 * Opens an interaction and lets the target handle it. Instant results close it again before returning,
	 * so only a Continuous result leaves a live handle in OutHandle.
	 */
	EKzInteractionResult BeginInteraction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FKzInteractionHandle& OutHandle);

	/** Ends a live interaction. Safe to call with a stale or already-ended handle. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndInteraction(FKzInteractionHandle Handle, EKzInteractionEndReason Reason);

	/** Ends every interaction an interactor is engaged in. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndInteractionsFor(const UKzInteractorComponent* Interactor, EKzInteractionEndReason Reason);

	/** Ends every interaction targeting an interactable. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndInteractionsOn(const UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason);

	/** Returns true while the handle names a live interaction. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionActive(FKzInteractionHandle Handle) const { return Interactions.Contains(Handle); }

	/** Looks up a live interaction. Returns null once it has ended. */
	const FKzInteraction* FindInteraction(FKzInteractionHandle Handle) const { return Interactions.Find(Handle); }

	/** The interaction this interactor is engaged in, if any. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FKzInteractionHandle FindInteractionFor(const UKzInteractorComponent* Interactor) const;

	/** Every live interaction targeting this interactable. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	TArray<FKzInteractionHandle> FindInteractionsOn(const UKzInteractableComponent* Interactable) const;

	/** Number of live interactions targeting this interactable, without building an array. */
	int32 CountInteractionsOn(const UKzInteractableComponent* Interactable) const;

	/** True if this actor is engaged in any interaction with the given interactable. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsActorInteractingWith(const AActor* Actor, const UKzInteractableComponent* Interactable) const;

	// ==========================================
	// SPATIAL INDEX
	// ==========================================

	/** Registers an interactable component into the spatial grid. Called by the component's Activate. */
	void RegisterInteractable(UKzInteractableComponent* Component);

	/** Unregisters an interactable, ending anything still targeting it. Called by the component's EndPlay. */
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

private:
	/** Drops interactions whose ends died, left the keep-alive range, or stopped meeting their conditions. */
	void ValidateInteractions();
};
