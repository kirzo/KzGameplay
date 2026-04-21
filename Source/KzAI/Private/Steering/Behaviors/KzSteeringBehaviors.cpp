// Copyright 2026 kirzo

#include "Steering/Behaviors/KzSteeringBehaviors.h"
#include "Steering/KzSteeringLibrary.h"
#include "Steering/KzSteeringAgent.h"
#include "Steering/KzSteeringComponent.h"
#include "Actors/KzAreaNetwork.h"
#include "Sensors/KzSpatialSenseSubsystem.h"
#include "Sensors/KzSensableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

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

	// Initialization: Pick the very first target
	if (!bHasTarget && !bIsWaiting)
	{
		PickNewTarget(AgentPos, MaxSpeed);
	}

	// Handle Waiting State
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

	const float DistSq = bForce2D ? FVector::DistSquared2D(AgentPos, CurrentWanderTarget) : FVector::DistSquared(AgentPos, CurrentWanderTarget);
	const bool bReachedByDistance = DistSq < (AcceptanceRadius * AcceptanceRadius);
	
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

	// Moving towards target
	if (bHasTarget)
	{
		TimeSpentMoving += DeltaTime;
	}

	const float SlowingRadius = AcceptanceRadius * 2.0f;
	return UKzSteeringLibrary::Arrive(AgentPos, CurrentWanderTarget, AgentVel, MaxSpeed, SlowingRadius, bForce2D);
}

void UKzSteeringBehavior_WanderArea::PickNewTarget(const FVector& AgentPos, float AgentMaxSpeed)
{
	CurrentWanderTarget = AreaNetwork->GetRandomPointInside(10);
	bHasTarget = true;
	TimeSpentMoving = 0.0f;

	if (AgentMaxSpeed > 5.0f)
	{
		// Calculate how long it should take in a perfect straight line
		const float StraightLineDist = bForce2D ? FVector::DistXY(AgentPos, CurrentWanderTarget) : FVector::Distance(AgentPos, CurrentWanderTarget);
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

FVector UKzSteeringBehavior_ObstacleAvoidance::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	UWorld* World = OwnerComponent->GetWorld();
	AActor* OwnerActor = OwnerComponent->GetOwner();

	if (!World || !OwnerActor)
	{
		return FVector::ZeroVector;
	}

	const FVector AgentPos = Agent->GetAgentLocation();
	const FVector AgentVel = Agent->GetAgentVelocity();
	const float MaxSpeed = Agent->GetAgentMaxSpeed();

	// Extract the exact Collision Profile Name from the Agent's Root Component.
	FName AgentCollisionProfile = UCollisionProfile::Pawn_ProfileName;
	if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
	{
		AgentCollisionProfile = PrimitiveComp->GetCollisionProfileName();
	}

	// Use a slightly smaller radius than the actual agent to allow tight navigation without scraping.
	const float FeelerRadius = FMath::Max(10.0f, Agent->GetAgentRadius() * 0.8f);

	// Use velocity direction if moving, otherwise fallback to actor's forward vector.
	FVector AgentDir = AgentVel.IsNearlyZero() ? OwnerActor->GetActorForwardVector() : AgentVel.GetSafeNormal();

	if (bForce2D)
	{
		AgentDir.Z = 0.0f;
		AgentDir.Normalize();
	}

	FVector TargetDesiredVelocity = FVector::ZeroVector;

	// Calculate Hysteresis and Lengths
	const float CurrentFeelerLength = bIsAvoiding ? (FeelerLength * HysteresisMultiplier) : FeelerLength;
	const float SafeAngle = FMath::Clamp(FeelerAngle, 1.0f, 89.0f);
	const float SideFeelerLength = CurrentFeelerLength / FMath::Cos(FMath::DegreesToRadians(SafeAngle));

	// Define standard horizontal feeler directions (Center, Left, Right)
	const FVector FeelerCenter = AgentDir;
	const FVector FeelerLeft = AgentDir.RotateAngleAxis(-SafeAngle, FVector::UpVector);
	const FVector FeelerRight = AgentDir.RotateAngleAxis(SafeAngle, FVector::UpVector);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ObstacleAvoidanceTrace), false, OwnerActor);
	FCollisionShape FeelerShape = FCollisionShape::MakeSphere(FeelerRadius);
	FHitResult Hit;

	bool bHitThisFrame = false;

	// Lambda helper to process a single sweep feeler
	auto ProcessFeeler = [&](const FVector& Direction, float Length) -> FVector
		{
			const FVector End = AgentPos + (Direction * Length);

			// Use Sweep instead of LineTrace to give the feeler "thickness" and prevent jitter.
			if (World->SweepSingleByProfile(Hit, AgentPos, End, FQuat::Identity, AgentCollisionProfile, FeelerShape, TraceParams))
			{
				bHitThisFrame = true;

				// Project the agent's direction onto the wall's plane to find the parallel glide path.
				FVector ParallelDir = FVector::VectorPlaneProject(AgentDir, Hit.ImpactNormal).GetSafeNormal();

				const float DistanceRatio = 1.0f - (Hit.Distance / Length);
				FVector Repulsion;

				// If the dot product is positive, the agent is already flying parallel to or away from the wall.
				if (FVector::DotProduct(AgentDir, Hit.ImpactNormal) > 0.05f)
				{
					// Provide a "cushion" force: Glide forward and push slightly outward to counter any inward Seek forces.
					Repulsion = ParallelDir + (Hit.ImpactNormal * 0.2f);

					if (bShowDebug)
					{
						DrawDebugLine(World, AgentPos, Hit.ImpactPoint, FColor::Orange, false, -1.0f, 0, 1.0f);
					}
				}
				else
				{
					// The agent is heading into the wall. Blend the parallel glide with the outward normal push.
					Repulsion = FMath::Lerp(ParallelDir, Hit.ImpactNormal, DistanceRatio);

					if (bShowDebug)
					{
						DrawDebugLine(World, AgentPos, Hit.ImpactPoint, FColor::Red, false, -1.0f, 0, 2.0f);
					}
				}

				if (bForce2D)
				{
					Repulsion.Z = 0.0f;
				}

				Repulsion.Normalize();
				Repulsion *= DistanceRatio;

				if (bShowDebug)
				{
					DrawDebugBox(World, Hit.ImpactPoint, FVector(5.0f), FColor::Red, false, -1.0f, 0, 2.0f);
					const FVector RepulsionEnd = Hit.ImpactPoint + (Repulsion * 100.0f);
					DrawDebugLine(World, Hit.ImpactPoint, RepulsionEnd, FColor::Yellow, false, -1.0f, 0, 3.0f);
				}

				return Repulsion;
			}

			if (bShowDebug)
			{
				DrawDebugLine(World, AgentPos, End, FColor::Green, false, -1.0f, 0, 1.0f);
			}

			return FVector::ZeroVector;
		};

	// Accumulate repulsive forces from the primary horizontal feelers
	TargetDesiredVelocity += ProcessFeeler(FeelerCenter, CurrentFeelerLength);
	TargetDesiredVelocity += ProcessFeeler(FeelerLeft, SideFeelerLength);
	TargetDesiredVelocity += ProcessFeeler(FeelerRight, SideFeelerLength);

	// 3D Vertical Awareness (Pitch Feelers)
	// Only apply if we are not restricted to 2D and there is significant vertical movement.
	if (!bForce2D && FMath::Abs(AgentVel.Z) > 10.0f)
	{
		// Calculate the agent's Right Vector based on its current velocity direction to act as the pitch axis.
		FVector AgentRight = FVector::CrossProduct(FVector::UpVector, AgentDir);
		if (AgentRight.IsNearlyZero())
		{
			// Fallback if flying perfectly straight up or down.
			AgentRight = OwnerActor->GetActorRightVector();
		}
		else
		{
			AgentRight.Normalize();
		}

		// If ascending (Z > 0), pitch feelers DOWN (+SafeAngle).
		// If descending (Z < 0), pitch feelers UP (-SafeAngle).
		const float PitchAngle = (AgentVel.Z > 0.0f) ? SafeAngle : -SafeAngle;

		// Create the new pitched base direction.
		const FVector PitchedDir = AgentDir.RotateAngleAxis(PitchAngle, AgentRight);

		// Generate the 3 extra feelers from this pitched base.
		const FVector PitchedCenter = PitchedDir;
		const FVector PitchedLeft = PitchedDir.RotateAngleAxis(-SafeAngle, FVector::UpVector);
		const FVector PitchedRight = PitchedDir.RotateAngleAxis(SafeAngle, FVector::UpVector);

		// Accumulate forces from the vertical awareness feelers.
		// We use slightly shorter lengths for the vertical ones to prevent them from hitting the ground too early when flying low.
		TargetDesiredVelocity += ProcessFeeler(PitchedCenter, CurrentFeelerLength * 0.8f);
		TargetDesiredVelocity += ProcessFeeler(PitchedLeft, SideFeelerLength * 0.8f);
		TargetDesiredVelocity += ProcessFeeler(PitchedRight, SideFeelerLength * 0.8f);
	}

	// Update Hysteresis State for the next frame
	bIsAvoiding = bHitThisFrame;

	// Scale the target force to match steering agent standards
	if (!TargetDesiredVelocity.IsNearlyZero())
	{
		TargetDesiredVelocity.Normalize();
		TargetDesiredVelocity *= MaxSpeed;
	}

	// Apply conditional smoothing (Inertia/Decay)
	if (ForceSmoothingSpeed > 0.0f && DeltaTime > 0.0f)
	{
		const float SmoothingSpeed = bIsAvoiding ? 2.0f * ForceSmoothingSpeed : ForceSmoothingSpeed;
		LastSmoothedVelocity = FMath::VInterpTo(LastSmoothedVelocity, TargetDesiredVelocity, DeltaTime, SmoothingSpeed);
	}
	else
	{
		LastSmoothedVelocity = TargetDesiredVelocity;
	}

	if (bShowDebug && !LastSmoothedVelocity.IsNearlyZero())
	{
		const FVector SmoothedEnd = AgentPos + (LastSmoothedVelocity.GetSafeNormal() * 150.0f);
		DrawDebugLine(World, AgentPos, SmoothedEnd, FColor::Cyan, false, -1.0f, 0, 4.0f);
	}

	return LastSmoothedVelocity;
}