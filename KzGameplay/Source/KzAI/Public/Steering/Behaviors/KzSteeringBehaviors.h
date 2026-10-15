// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Steering/KzSteeringBehavior.h"
#include "GameplayTagContainer.h"
#include "KzSteeringBehaviors.generated.h"

class AKzAreaNetwork;

UCLASS(DisplayName = "Seek")
class KZAI_API UKzSteeringBehavior_Seek : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** Optional actor to follow. If null, TargetLocation is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seek", meta = (ExposeOnSpawn))
	TObjectPtr<AActor> TargetActor;

	/** Fallback location if TargetActor is null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seek", meta = (ExposeOnSpawn))
	FVector TargetLocation = FVector::ZeroVector;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Flee")
class KZAI_API UKzSteeringBehavior_Flee : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ExposeOnSpawn))
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ExposeOnSpawn))
	FVector TargetLocation = FVector::ZeroVector;

	/**
	 * Offset applied to the threat's location before calculating the flee force.
	 * Use a negative Z value (e.g., Z = -500) to make flying agents instinctively flee upwards.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ExposeOnSpawn))
	FVector TargetOffset = FVector::ZeroVector;

	/** If the agent is further than this from the threat, the behavior produces no force (0 = infinite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ExposeOnSpawn))
	float PanicDistance = 1000.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Arrive")
class KZAI_API UKzSteeringBehavior_Arrive : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrive", meta = (ExposeOnSpawn))
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrive", meta = (ExposeOnSpawn))
	FVector TargetLocation = FVector::ZeroVector;

	/** The distance at which the agent starts slowing down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrive", meta = (ExposeOnSpawn, ClampMin = "10.0"))
	float SlowingRadius = 300.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Wander Area")
class KZAI_API UKzSteeringBehavior_WanderArea : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** The specific area network to roam inside. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander", meta = (ExposeOnSpawn))
	TObjectPtr<AKzAreaNetwork> AreaNetwork;

	/** Distance at which the agent considers a point reached and requests a new one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander", meta = (ExposeOnSpawn))
	float AcceptanceRadius = 150.0f;

	/** Minimum time (in seconds) the agent will wait before picking the next point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander|Timing", meta = (ExposeOnSpawn, ClampMin = "0.0"))
	float MinWaitTime = 1.0f;

	/** Maximum time (in seconds) the agent will wait before picking the next point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander|Timing", meta = (ExposeOnSpawn, ClampMin = "0.0"))
	float MaxWaitTime = 3.0f;

	/**
	 * Multiplier for the expected travel time.
	 * If it takes longer than (Distance/Speed) * Multiplier to arrive, it aborts and rests.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander|Timing", meta = (ExposeOnSpawn, ClampMin = "1.0"))
	float TimeoutMultiplier = 3.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;

protected:
	void PickNewTarget(const FVector& AgentPos, float AgentMaxSpeed);

	UPROPERTY(Transient)
	FVector CurrentWanderTarget = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasTarget = false;

	UPROPERTY(Transient)
	bool bIsWaiting = false;

	UPROPERTY(Transient)
	float CurrentWaitDuration = 0.0f;

	UPROPERTY(Transient)
	float TimeWaited = 0.0f;

	UPROPERTY(Transient)
	float TimeSpentMoving = 0.0f;

	UPROPERTY(Transient)
	float MaxTimeToReachTarget = 0.0f;
};

UCLASS(DisplayName = "Flocking")
class KZAI_API UKzSteeringBehavior_Flocking : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** Tags to identify flock mates (e.g., "Creature.Bird") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking", meta = (ExposeOnSpawn))
	FGameplayTagQuery FlockMateQuery;

	/** Radius to search for neighbors using the spatial grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking", meta = (ExposeOnSpawn))
	float SearchRadius = 500.0f;

	/** Radius at which agents start to actively repel each other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking", meta = (ExposeOnSpawn, ClampMin = "10.0"))
	float SeparationRadius = 120.0f;

	/** Field of View in degrees. Agents outside this cone are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking", meta = (ExposeOnSpawn, ClampMin = "0.0", ClampMax = "360.0"))
	float PeripheralVisionAngle = 270.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking|Weights", meta = (ExposeOnSpawn))
	float SeparationWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking|Weights", meta = (ExposeOnSpawn))
	float AlignmentWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking|Weights", meta = (ExposeOnSpawn))
	float CohesionWeight = 1.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Collision Avoidance")
class KZAI_API UKzSteeringBehavior_CollisionAvoidance : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** Query used to find other moving agents to avoid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	FGameplayTagQuery AgentQuery;

	/** How far around the agent to look for potential collisions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance", meta = (ClampMin = "100.0"))
	float SearchRadius = 500.0f;

	/** The time horizon (in seconds) to predict collisions. E.g., 2.0 means "will I hit something in the next 2 seconds?" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance", meta = (ClampMin = "0.1"))
	float MaxLookAheadTime = 2.0f;

	/** Multiplier applied to LookAheadTime and AgentRadius while actively avoiding to prevent jitter (hysteresis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance", meta = (ClampMin = "1.0"))
	float HysteresisMultiplier = 2.0f;

	/** How quickly the avoidance force decays or adjusts to new obstacles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance", meta = (ClampMin = "0.0"))
	float ForceSmoothingSpeed = 1.0f;

	/** If true, draws the sensor radius and lines to detected neighbors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance|Debug")
	bool bShowDebug = false;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;

private:
	/** Tracks if the agent was avoiding an obstacle in the previous frame to apply hysteresis. */
	UPROPERTY(Transient)
	bool bIsAvoiding = false;

	/** Stores the desired velocity applied in the previous frame to interpolate smoothly. */
	UPROPERTY(Transient)
	FVector LastSmoothedVelocity = FVector::ZeroVector;
};

UCLASS(DisplayName = "Obstacle Avoidance (Environment)")
class KZAI_API UKzSteeringBehavior_ObstacleAvoidance : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** Base length of the forward-facing raycast. Side feelers will be dynamically extended to match this forward depth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Avoidance", meta = (ExposeOnSpawn))
	float FeelerLength = 500.0f;

	/** Angle (in degrees) for the side feelers relative to the agent's forward direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Avoidance", meta = (ExposeOnSpawn, ClampMin = "1.0", ClampMax = "89.0"))
	float FeelerAngle = 30.0f;

	/** Multiplier applied to the feeler length while actively avoiding an obstacle to prevent jitter (hysteresis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Avoidance", meta = (ExposeOnSpawn, ClampMin = "1.0"))
	float HysteresisMultiplier = 1.5f;

	/** How quickly the avoidance force decays or adjusts to new obstacles. Lower values mean smoother, wider turns around corners. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Avoidance", meta = (ExposeOnSpawn, ClampMin = "0.0"))
	float ForceSmoothingSpeed = 4.0f;

	/** If true, draws debug lines for the feelers in the viewport to visualize hits and repulsive forces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Avoidance|Debug")
	bool bShowDebug = false;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;

private:
	/** Tracks if the agent was avoiding an obstacle in the previous frame to apply hysteresis. */
	UPROPERTY(Transient)
	bool bIsAvoiding = false;

	/** Stores the desired velocity applied in the previous frame to interpolate smoothly and prevent sharp turns. */
	UPROPERTY(Transient)
	FVector LastSmoothedVelocity = FVector::ZeroVector;
};