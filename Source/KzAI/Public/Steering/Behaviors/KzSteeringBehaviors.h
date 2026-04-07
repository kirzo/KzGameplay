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
	float SearchRadius = 800.0f;

	/** The time horizon (in seconds) to predict collisions. E.g., 2.0 means "will I hit something in the next 2 seconds?" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance", meta = (ClampMin = "0.1"))
	float MaxLookAheadTime = 2.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};