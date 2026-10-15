// Copyright 2026 kirzo

#include "Steering/Behaviors/KzSteeringBehavior_FollowSpline.h"
#include "Steering/KzSteeringLibrary.h"
#include "Steering/KzSteeringAgent.h"
#include "Steering/KzSteeringComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UKzSteeringBehavior_FollowSpline::UKzSteeringBehavior_FollowSpline()
{
	bForce2D = false;
}

FVector UKzSteeringBehavior_FollowSpline::ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime)
{
	UWorld* World = OwnerComponent->GetWorld();
	AActor* OwnerActor = OwnerComponent->GetOwner();

	if (!World || !OwnerActor)
	{
		return FVector::ZeroVector;
	}

	// Resolve the spline component reference.
	USplineComponent* Spline = SplineReference.GetComponent<USplineComponent>(OwnerActor);
	if (!Spline)
	{
		return FVector::ZeroVector;
	}

	const FVector AgentPos = Agent->GetAgentLocation();
	const FVector AgentVel = Agent->GetAgentVelocity();
	const float MaxSpeed = Agent->GetAgentMaxSpeed();

	// 1. Predict future position based on current velocity
	FVector PredictedPos = AgentPos + (AgentVel * PredictionTime);

	// 2. Find the closest point on the spline to the predicted position
	const float ClosestInputKey = Spline->FindInputKeyClosestToWorldLocation(PredictedPos);
	const float ClosestDistance = Spline->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);

	// 3. Place the 'carrot' target ahead on the spline
	const float SplineLength = Spline->GetSplineLength();
	const float DirectionMult = bFollowForward ? 1.0f : -1.0f;
	float TargetDistance = ClosestDistance + (CarrotDistance * DirectionMult);

	FVector TargetPos = FVector::ZeroVector;
	FVector DesiredVelocity = FVector::ZeroVector;

	// 4. Handle End of Spline Logic
	if (Spline->IsClosedLoop())
	{
		// Wrap around seamlessly
		while (TargetDistance > SplineLength) TargetDistance -= SplineLength;
		while (TargetDistance < 0.0f) TargetDistance += SplineLength;

		TargetPos = Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
		DesiredVelocity = UKzSteeringLibrary::Seek(AgentPos, TargetPos, AgentVel, MaxSpeed, bForce2D);
	}
	else
	{
		if (bFollowForward && TargetDistance >= SplineLength)
		{
			switch (EndBehavior)
			{
			case EKzSplineEndBehavior::Reverse:
				bFollowForward = false;
				TargetDistance = SplineLength - (TargetDistance - SplineLength); // Bounce back slightly
				TargetPos = Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
				DesiredVelocity = UKzSteeringLibrary::Seek(AgentPos, TargetPos, AgentVel, MaxSpeed, bForce2D);
				break;

			case EKzSplineEndBehavior::Stop:
				TargetPos = Spline->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);
				// Use Arrive instead of Seek to smoothly brake at the end of the line
				DesiredVelocity = UKzSteeringLibrary::Arrive(AgentPos, TargetPos, AgentVel, MaxSpeed, SlowingRadius, bForce2D);
				break;

			case EKzSplineEndBehavior::Continue:
			{
				TargetPos = AgentPos + (AgentVel.GetSafeNormal() * CarrotDistance);
				DesiredVelocity = UKzSteeringLibrary::Seek(AgentPos, TargetPos, AgentVel, MaxSpeed, bForce2D);
			}
			break;
			}
		}
		else if (!bFollowForward && TargetDistance <= 0.0f)
		{
			switch (EndBehavior)
			{
			case EKzSplineEndBehavior::Reverse:
				bFollowForward = true;
				TargetDistance = FMath::Abs(TargetDistance); // Bounce back forward
				TargetPos = Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
				DesiredVelocity = UKzSteeringLibrary::Seek(AgentPos, TargetPos, AgentVel, MaxSpeed, bForce2D);
				break;

			case EKzSplineEndBehavior::Stop:
				TargetPos = Spline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
				DesiredVelocity = UKzSteeringLibrary::Arrive(AgentPos, TargetPos, AgentVel, MaxSpeed, SlowingRadius, bForce2D);
				break;

			case EKzSplineEndBehavior::Continue:
			{
				TargetPos = AgentPos + (AgentVel.GetSafeNormal() * CarrotDistance);
				DesiredVelocity = UKzSteeringLibrary::Seek(AgentPos, TargetPos, AgentVel, MaxSpeed, bForce2D);
			}
			break;
			}
		}
		else
		{
			// Normal path following within spline bounds
			TargetPos = Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
			DesiredVelocity = UKzSteeringLibrary::Seek(AgentPos, TargetPos, AgentVel, MaxSpeed, bForce2D);
		}
	}

	// 5. Visual Debugging
	if (bShowDebug)
	{
		FVector ClosestPos = Spline->GetLocationAtDistanceAlongSpline(ClosestDistance, ESplineCoordinateSpace::World);

		// Blue: Predicted position of the agent
		DrawDebugPoint(World, PredictedPos, 10.0f, FColor::Blue, false, -1.0f);

		// Cyan: Closest point on the spline to the prediction
		DrawDebugPoint(World, ClosestPos, 15.0f, FColor::Cyan, false, -1.0f);
		DrawDebugLine(World, PredictedPos, ClosestPos, FColor::Cyan, false, -1.0f, 0, 1.0f);

		// Orange/Yellow: The Carrot target and the steering line
		DrawDebugSphere(World, TargetPos, 25.0f, 12, FColor::Orange, false, -1.0f, 0, 2.0f);
		DrawDebugLine(World, AgentPos, TargetPos, FColor::Yellow, false, -1.0f, 0, 2.0f);
	}

	return DesiredVelocity;
}