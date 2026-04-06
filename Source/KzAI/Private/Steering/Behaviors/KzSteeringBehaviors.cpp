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
	const FVector TargetPos = TargetActor ? TargetActor->GetActorLocation() : TargetLocation;
	return UKzSteeringLibrary::Flee(Agent->GetAgentLocation(), TargetPos, Agent->GetAgentVelocity(), Agent->GetAgentMaxSpeed(), bForce2D);
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

	// Check if we need a new point
	if (!bHasTarget || FVector::DistSquared(AgentPos, CurrentWanderTarget) < (AcceptanceRadius * AcceptanceRadius))
	{
		CurrentWanderTarget = AreaNetwork->GetRandomPointInside(10);
		bHasTarget = true;
	}

	return UKzSteeringLibrary::Seek(AgentPos, CurrentWanderTarget, Agent->GetAgentVelocity(), Agent->GetAgentMaxSpeed(), bForce2D);
}

FVector UKzSteeringBehavior_Flocking::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	if (FlockMateQuery.IsEmpty()) return FVector::ZeroVector;

	UKzSpatialSenseSubsystem* SenseSubsystem = OwnerComponent->GetWorld()->GetSubsystem<UKzSpatialSenseSubsystem>();
	if (!SenseSubsystem) return FVector::ZeroVector;

	const FVector AgentPos = Agent->GetAgentLocation();
	const FVector AgentVel = Agent->GetAgentVelocity();
	const float MaxSpeed = Agent->GetAgentMaxSpeed();

	// 1. Query the grid for neighbors
	FKzShapeInstance QuerySphere = FKzShapeInstance::Make<FKzSphere>(SearchRadius);
	TArray<UKzSensableComponent*> Neighbors = SenseSubsystem->QuerySensables(QuerySphere, AgentPos, FQuat::Identity, FlockMateQuery);

	TArray<FVector> NeighborLocations;
	TArray<FVector> NeighborVelocities;
	NeighborLocations.Reserve(Neighbors.Num());
	NeighborVelocities.Reserve(Neighbors.Num());

	// 2. Extract Data (Ignoring self)
	AActor* OwnerActor = Cast<AActor>(OwnerComponent->GetOwner());

	for (UKzSensableComponent* NeighborComp : Neighbors)
	{
		AActor* NeighborActor = NeighborComp->GetOwner();
		if (!NeighborActor || NeighborActor == OwnerActor) continue;

		NeighborLocations.Add(NeighborComp->GetShapeLocation());
		NeighborVelocities.Add(NeighborActor->GetVelocity());
	}

	if (NeighborLocations.IsEmpty()) return FVector::ZeroVector;

	// 3. Delegate math to the Library
	FVector SeparationForce = UKzSteeringLibrary::Separation(AgentPos, AgentVel, NeighborLocations, bForce2D) * SeparationWeight;
	FVector AlignmentForce = UKzSteeringLibrary::Alignment(AgentVel, NeighborVelocities, MaxSpeed, bForce2D) * AlignmentWeight;
	FVector CohesionForce = UKzSteeringLibrary::Cohesion(AgentPos, AgentVel, NeighborLocations, MaxSpeed, bForce2D) * CohesionWeight;

	return SeparationForce + AlignmentForce + CohesionForce;
}