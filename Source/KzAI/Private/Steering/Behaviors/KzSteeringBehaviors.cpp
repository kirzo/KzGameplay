// Copyright 2026 kirzo

#include "Steering/Behaviors/KzSteeringBehaviors.h"
#include "Steering/KzSteeringLibrary.h"
#include "Steering/KzSteeringAgent.h"
#include "Steering/KzSteeringComponent.h"
#include "Actors/KzAreaNetwork.h"
#include "Sensors/KzSpatialSenseSubsystem.h"
#include "Sensors/KzSensableComponent.h"
#include "Engine/World.h"

FVector UKzSteeringBehavior_Seek::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	const FVector TargetPos = TargetActor ? TargetActor->GetActorLocation() : TargetLocation;
	return UKzSteeringLibrary::Seek(Agent->GetAgentLocation(), TargetPos, Agent->GetAgentVelocity(), Agent->GetAgentMaxSpeed(), bForce2D);
}

FVector UKzSteeringBehavior_Flee::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	FVector TargetPos = TargetActor ? TargetActor->GetActorLocation() : TargetLocation;
	const FVector AgentPos = Agent->GetAgentLocation();

	// 1. Check Panic Distance (abort early if safe)
	if (PanicDistance > 0.0f)
	{
		const float DistSq = FVector::DistSquared(AgentPos, TargetPos);
		if (DistSq > (PanicDistance * PanicDistance))
		{
			// The threat is too far, we don't need to apply any flee force
			return FVector::ZeroVector;
		}
	}

	// 2. Apply the offset to manipulate the escape trajectory
	// E.g. A negative Z offset creates an upward force.
	TargetPos += TargetOffset;

	// 3. Compute the steering force
	return UKzSteeringLibrary::Flee(AgentPos, TargetPos, Agent->GetAgentVelocity(), Agent->GetAgentMaxSpeed(), bForce2D);
}

FVector UKzSteeringBehavior_Arrive::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	const FVector TargetPos = TargetActor ? TargetActor->GetActorLocation() : TargetLocation;
	return UKzSteeringLibrary::Arrive(Agent->GetAgentLocation(), TargetPos, Agent->GetAgentVelocity(), Agent->GetAgentMaxSpeed(), SlowingRadius, bForce2D);
}

FVector UKzSteeringBehavior_WanderArea::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	if (!AreaNetwork) return FVector::ZeroVector;

	const FVector AgentPos = Agent->GetAgentLocation();
	const FVector AgentVel = Agent->GetAgentVelocity();
	const float MaxSpeed = Agent->GetAgentMaxSpeed();

	// 1. Initialization: Pick the very first target
	if (!bHasTarget && !bIsWaiting)
	{
		PickNewTarget(AgentPos, MaxSpeed);
	}

	// 2. Handle Waiting State
	if (bIsWaiting)
	{
		TimeWaited += DeltaTime;
		if (TimeWaited >= CurrentWaitDuration)
		{
			// Done waiting, pick a new target
			bIsWaiting = false;
			PickNewTarget(AgentPos, MaxSpeed);
		}
		else
		{
			return FVector::ZeroVector;
		}
	}

	// 3. Check conditions to stop moving (Reached physically or Timed out)
	const bool bReachedByDistance = FVector::DistSquared(AgentPos, CurrentWanderTarget) < (AcceptanceRadius * AcceptanceRadius);
	
	// Only apply timeout if the agent is inside the valid AreaNetwork.
	// This prevents the agent from giving up while returning from a long distance (e.g., after fleeing).
	const bool bReachedByTimeout = (MaxTimeToReachTarget > 0.0f) && (TimeSpentMoving >= MaxTimeToReachTarget) && AreaNetwork->IsPointInside(AgentPos);

	if (bHasTarget && (bReachedByDistance || bReachedByTimeout))
	{
		// Target reached or aborted! Start the resting phase.
		bHasTarget = false;
		bIsWaiting = true;
		TimeWaited = 0.0f;

		const float SafeMin = FMath::Min(MinWaitTime, MaxWaitTime);
		const float SafeMax = FMath::Max(MinWaitTime, MaxWaitTime);
		CurrentWaitDuration = FMath::RandRange(SafeMin, SafeMax);

		return FVector::ZeroVector;
	}

	// 4. Moving towards target
	if (bHasTarget)
	{
		TimeSpentMoving += DeltaTime;
	}

	return UKzSteeringLibrary::Seek(AgentPos, CurrentWanderTarget, AgentVel, MaxSpeed, bForce2D);
}

void UKzSteeringBehavior_WanderArea::PickNewTarget(const FVector& AgentPos, float AgentMaxSpeed)
{
	CurrentWanderTarget = AreaNetwork->GetRandomPointInside(10);
	bHasTarget = true;
	TimeSpentMoving = 0.0f;

	if (AgentMaxSpeed > 5.0f)
	{
		// Calculate how long it should take in a perfect straight line
		const float StraightLineDist = FVector::Distance(AgentPos, CurrentWanderTarget);
		const float ExpectedTime = StraightLineDist / AgentMaxSpeed;

		MaxTimeToReachTarget = ExpectedTime * TimeoutMultiplier;

		// Provide a bare minimum timeout (e.g. 1.5 seconds) just in case the target is weirdly close
		MaxTimeToReachTarget = FMath::Max(MaxTimeToReachTarget, 1.5f);
	}
	else
	{
		MaxTimeToReachTarget = 0.0f; // Failsafe if max speed is 0
	}
}

FVector UKzSteeringBehavior_Flocking::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	if (FlockMateQuery.IsEmpty()) return FVector::ZeroVector;

	UKzSpatialSenseSubsystem* SenseSubsystem = OwnerComponent->GetWorld()->GetSubsystem<UKzSpatialSenseSubsystem>();
	if (!SenseSubsystem) return FVector::ZeroVector;

	const FVector AgentPos = Agent->GetAgentLocation();
	const FVector AgentVel = Agent->GetAgentVelocity();
	const FVector AgentDir = Agent->GetAgentDirection();
	const float MaxSpeed = Agent->GetAgentMaxSpeed();
	const float AgentRadius = Agent->GetAgentRadius();

	FKzShapeInstance QuerySphere = FKzShapeInstance::Make<FKzSphere>(SearchRadius);
	TArray<UKzSensableComponent*> Neighbors = SenseSubsystem->QuerySensables(QuerySphere, AgentPos, FQuat::Identity, FlockMateQuery);

	TArray<FVector> NeighborLocations;
	TArray<FVector> NeighborVelocities;
	TArray<FVector> SeparationNeighbors;

	NeighborLocations.Reserve(Neighbors.Num());
	NeighborVelocities.Reserve(Neighbors.Num());
	SeparationNeighbors.Reserve(Neighbors.Num());

	AActor* OwnerActor = Cast<AActor>(OwnerComponent->GetOwner());

	// Convert FOV to Cosine for fast dot product checks
	const float VisionCos = FMath::Cos(FMath::DegreesToRadians(PeripheralVisionAngle * 0.5f));

	for (UKzSensableComponent* NeighborComp : Neighbors)
	{
		AActor* NeighborActor = NeighborComp->GetOwner();
		if (!NeighborActor || NeighborActor == OwnerActor) continue;

		FVector NeighborLoc = NeighborComp->GetShapeLocation();
		FVector ToNeighbor = NeighborLoc - AgentPos;
		if (bForce2D) ToNeighbor.Z = 0.0f;

		// 1. Field of View Check (Blind Spot)
		// Ignore neighbors strictly behind us, unless they are dangerously close
		const float DistSq = ToNeighbor.SizeSquared();
		if (DistSq > (SeparationRadius * SeparationRadius))
		{
			if (FVector::DotProduct(AgentDir, ToNeighbor.GetSafeNormal()) < VisionCos)
			{
				continue; // Cannot see this neighbor
			}
		}

		NeighborLocations.Add(NeighborLoc);
		NeighborVelocities.Add(NeighborActor->GetVelocity());

		// 2. Intimate space check for Separation
		if (DistSq < (SeparationRadius * SeparationRadius))
		{
			SeparationNeighbors.Add(NeighborLoc);
		}
	}

	if (NeighborLocations.IsEmpty()) return FVector::ZeroVector;

	FVector SeparationForce = UKzSteeringLibrary::Separation(AgentPos, AgentVel, SeparationNeighbors, MaxSpeed, AgentRadius, bForce2D) * SeparationWeight;
	FVector AlignmentForce = UKzSteeringLibrary::Alignment(AgentVel, NeighborVelocities, MaxSpeed, bForce2D) * AlignmentWeight;
	FVector CohesionForce = UKzSteeringLibrary::Cohesion(AgentPos, AgentVel, NeighborLocations, MaxSpeed, bForce2D) * CohesionWeight;

	return SeparationForce + AlignmentForce + CohesionForce;
}

FVector UKzSteeringBehavior_CollisionAvoidance::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	if (AgentQuery.IsEmpty()) return FVector::ZeroVector;

	UKzSpatialSenseSubsystem* SenseSubsystem = OwnerComponent->GetWorld()->GetSubsystem<UKzSpatialSenseSubsystem>();
	if (!SenseSubsystem) return FVector::ZeroVector;

	const FVector AgentPos = Agent->GetAgentLocation();
	const FVector AgentVel = Agent->GetAgentVelocity();
	const float MaxSpeed = Agent->GetAgentMaxSpeed();
	const float AgentRadius = Agent->GetAgentRadius();

	FKzShapeInstance QuerySphere = FKzShapeInstance::Make<FKzSphere>(SearchRadius);
	TArray<UKzSensableComponent*> Neighbors = SenseSubsystem->QuerySensables(QuerySphere, AgentPos, FQuat::Identity, AgentQuery);

	TArray<FVector> NeighborLocations;
	TArray<FVector> NeighborVelocities;
	NeighborLocations.Reserve(Neighbors.Num());
	NeighborVelocities.Reserve(Neighbors.Num());

	AActor* OwnerActor = Cast<AActor>(OwnerComponent->GetOwner());

	for (UKzSensableComponent* NeighborComp : Neighbors)
	{
		AActor* NeighborActor = NeighborComp->GetOwner();
		if (!NeighborActor || NeighborActor == OwnerActor) continue;

		NeighborLocations.Add(NeighborComp->GetShapeLocation());
		NeighborVelocities.Add(NeighborActor->GetVelocity());
	}

	if (NeighborLocations.IsEmpty()) return FVector::ZeroVector;

	return UKzSteeringLibrary::CollisionAvoidance(AgentPos, AgentVel, AgentRadius, NeighborLocations, NeighborVelocities, MaxLookAheadTime, MaxSpeed, bForce2D);
}