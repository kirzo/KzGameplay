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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seek")
	TObjectPtr<AActor> TargetActor;

	/** Fallback location if TargetActor is null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seek")
	FVector TargetLocation = FVector::ZeroVector;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Flee")
class KZAI_API UKzSteeringBehavior_Flee : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	FVector TargetLocation = FVector::ZeroVector;

	/** If the agent is further than this from the threat, the behavior produces no force (0 = infinite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	float PanicDistance = 1000.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Arrive")
class KZAI_API UKzSteeringBehavior_Arrive : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrive")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrive")
	FVector TargetLocation = FVector::ZeroVector;

	/** The distance at which the agent starts slowing down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrive", meta = (ClampMin = "10.0"))
	float SlowingRadius = 300.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};

UCLASS(DisplayName = "Wander Area")
class KZAI_API UKzSteeringBehavior_WanderArea : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** The specific area network to roam inside. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander")
	TObjectPtr<AKzAreaNetwork> AreaNetwork;

	/** Distance at which the agent considers a point reached and requests a new one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wander")
	float AcceptanceRadius = 150.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;

protected:
	UPROPERTY(Transient)
	FVector CurrentWanderTarget = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasTarget = false;
};

UCLASS(DisplayName = "Flocking")
class KZAI_API UKzSteeringBehavior_Flocking : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	/** Tags to identify flock mates (e.g., "Creature.Bird") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking")
	FGameplayTagQuery FlockMateQuery;

	/** Radius to search for neighbors using the spatial grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking")
	float SearchRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking|Weights")
	float SeparationWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking|Weights")
	float AlignmentWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flocking|Weights")
	float CohesionWeight = 1.0f;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};