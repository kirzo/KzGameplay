// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzSteeringLibrary.generated.h"

/**
 * Pure, stateless library for calculating steering forces based on Craig Reynolds' algorithms.
 * All functions return a raw steering force vector (DesiredVelocity - CurrentVelocity).
 */
UCLASS()
class KZAI_API UKzSteeringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Calculates the steering force to move directly towards a target. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering")
	static FVector Seek(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, bool bForce2D = false);

	/** Calculates the steering force to arrive smoothly at a target, slowing down inside the radius. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering")
	static FVector Arrive(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, float SlowingRadius, bool bForce2D = false);

	/** Calculates the steering force to move away from a target. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering")
	static FVector Flee(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, bool bForce2D = false);

	/** Predicts the evader's future position and seeks towards it. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering")
	static FVector Pursuit(const FVector& CurrentLocation, const FVector& CurrentVelocity, const FVector& EvaderLocation, const FVector& EvaderVelocity, float MaxSpeed, bool bForce2D = false);

	/** Predicts the pursuer's future position and flees from it. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering")
	static FVector Evade(const FVector& CurrentLocation, const FVector& CurrentVelocity, const FVector& PursuerLocation, const FVector& PursuerVelocity, float MaxSpeed, bool bForce2D = false);

	/** Calculates the force to move to the midpoint of two moving targets. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering")
	static FVector Interpose(const FVector& CurrentLocation, const FVector& CurrentVelocity, const FVector& TargetALocation, const FVector& TargetAVelocity, const FVector& TargetBLocation, const FVector& TargetBVelocity, float MaxSpeed, bool bForce2D = false);

	/** Flocking: Steers away from neighbors to avoid crowding. Force is inversely proportional to distance. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering|Flocking")
	static FVector Separation(const FVector& CurrentLocation, const FVector& CurrentVelocity, const TArray<FVector>& NeighborLocations, float MaxSpeed, bool bForce2D = false);

	/** Flocking: Steers to match the average velocity (heading and speed) of neighbors. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering|Flocking")
	static FVector Alignment(const FVector& CurrentVelocity, const TArray<FVector>& NeighborVelocities, float MaxSpeed, bool bForce2D = false);

	/** Flocking: Steers towards the center of mass of neighbors. */
	UFUNCTION(BlueprintPure, Category = "KzAI|Steering|Flocking")
	static FVector Cohesion(const FVector& CurrentLocation, const FVector& CurrentVelocity, const TArray<FVector>& NeighborLocations, float MaxSpeed, bool bForce2D = false);
};