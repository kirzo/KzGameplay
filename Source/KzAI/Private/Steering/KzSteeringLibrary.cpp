// Copyright 2026 kirzo

#include "Steering/KzSteeringLibrary.h"

FVector UKzSteeringLibrary::Seek(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, bool bForce2D)
{
	FVector ToTarget = TargetLocation - CurrentLocation;
	if (bForce2D) ToTarget.Z = 0.0f;

	FVector DesiredVelocity = ToTarget.GetSafeNormal() * MaxSpeed;
	if (bForce2D) DesiredVelocity.Z = 0.0f;

	return DesiredVelocity;
}

FVector UKzSteeringLibrary::Arrive(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, float SlowingRadius, bool bForce2D)
{
	FVector ToTarget = TargetLocation - CurrentLocation;
	if (bForce2D) ToTarget.Z = 0.0f;

	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return -CurrentVelocity;
	}

	const float RampedSpeed = MaxSpeed * (Distance / FMath::Max(SlowingRadius, 1.0f));
	const float ClippedSpeed = FMath::Min(RampedSpeed, MaxSpeed);

	FVector DesiredVelocity = (ToTarget / Distance) * ClippedSpeed;
	if (bForce2D) DesiredVelocity.Z = 0.0f;

	return DesiredVelocity;
}

FVector UKzSteeringLibrary::Flee(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, bool bForce2D)
{
	FVector AwayFromTarget = CurrentLocation - TargetLocation;
	if (bForce2D) AwayFromTarget.Z = 0.0f;

	FVector DesiredVelocity = AwayFromTarget.GetSafeNormal() * MaxSpeed;
	if (bForce2D) DesiredVelocity.Z = 0.0f;

	return DesiredVelocity;
}

FVector UKzSteeringLibrary::Pursuit(const FVector& CurrentLocation, const FVector& CurrentVelocity, const FVector& EvaderLocation, const FVector& EvaderVelocity, float MaxSpeed, bool bForce2D)
{
	FVector ToEvader = EvaderLocation - CurrentLocation;
	if (bForce2D) ToEvader.Z = 0.0f;

	const float Distance = ToEvader.Size();

	// If the evader is ahead and facing us, we can just seek directly to their current position
	const FVector Heading = CurrentVelocity.GetSafeNormal();
	const FVector EvaderHeading = EvaderVelocity.GetSafeNormal();
	const float RelativeHeading = FVector::DotProduct(Heading, EvaderHeading);

	if (RelativeHeading < -0.95f && FVector::DotProduct(Heading, ToEvader.GetSafeNormal()) > 0.0f)
	{
		return Seek(CurrentLocation, EvaderLocation, CurrentVelocity, MaxSpeed, bForce2D);
	}

	// Predict future position
	const float PredictionTime = Distance / (MaxSpeed + EvaderVelocity.Size());
	const FVector PredictedLocation = EvaderLocation + (EvaderVelocity * PredictionTime);

	return Seek(CurrentLocation, PredictedLocation, CurrentVelocity, MaxSpeed, bForce2D);
}

FVector UKzSteeringLibrary::Evade(const FVector& CurrentLocation, const FVector& CurrentVelocity, const FVector& PursuerLocation, const FVector& PursuerVelocity, float MaxSpeed, bool bForce2D)
{
	FVector ToPursuer = PursuerLocation - CurrentLocation;
	if (bForce2D) ToPursuer.Z = 0.0f;

	const float Distance = ToPursuer.Size();
	const float PredictionTime = Distance / (MaxSpeed + PursuerVelocity.Size());
	const FVector PredictedLocation = PursuerLocation + (PursuerVelocity * PredictionTime);

	return Flee(CurrentLocation, PredictedLocation, CurrentVelocity, MaxSpeed, bForce2D);
}

FVector UKzSteeringLibrary::Interpose(const FVector& CurrentLocation, const FVector& CurrentVelocity, const FVector& TargetALocation, const FVector& TargetAVelocity, const FVector& TargetBLocation, const FVector& TargetBVelocity, float MaxSpeed, bool bForce2D)
{
	// Find the midpoint between A and B
	FVector Midpoint = (TargetALocation + TargetBLocation) / 2.0f;

	// Time it takes us to get to the midpoint
	const float TimeToReachMidpoint = FVector::Distance(CurrentLocation, Midpoint) / MaxSpeed;

	// Predict where A and B will be
	const FVector PredictedA = TargetALocation + (TargetAVelocity * TimeToReachMidpoint);
	const FVector PredictedB = TargetBLocation + (TargetBVelocity * TimeToReachMidpoint);

	// The new midpoint
	FVector PredictedMidpoint = (PredictedA + PredictedB) / 2.0f;

	return Seek(CurrentLocation, PredictedMidpoint, CurrentVelocity, MaxSpeed, bForce2D);
}

FVector UKzSteeringLibrary::Separation(const FVector& CurrentLocation, const FVector& CurrentVelocity, const TArray<FVector>& NeighborLocations, float MaxSpeed, float AgentRadius, bool bForce2D)
{
	FVector PushForce = FVector::ZeroVector;
	int32 Count = 0;

	for (const FVector& NeighborLoc : NeighborLocations)
	{
		FVector ToAgent = CurrentLocation - NeighborLoc;
		if (bForce2D) ToAgent.Z = 0.0f;

		const float Distance = ToAgent.Size();

		if (Distance > UE_KINDA_SMALL_NUMBER)
		{
			//Scale it based on how close they are compared to our radius
			PushForce += ToAgent / AgentRadius;
			// Weight the push force inversely proportional to the distance.
			// Closer neighbors push significantly harder.
			//PushForce += ToAgent.GetUnsafeNormal() / Distance;
			Count++;
		}
	}

	if (Count == 0) return FVector::ZeroVector;

	// Average out the accumulated forces
	PushForce /= Count;

	// Scale the final normalized escape vector by MaxSpeed
	FVector DesiredVelocity = PushForce * MaxSpeed;
	if (bForce2D) DesiredVelocity.Z = 0.0f;

	return DesiredVelocity;
}

FVector UKzSteeringLibrary::Alignment(const FVector& CurrentVelocity, const TArray<FVector>& NeighborVelocities, float MaxSpeed, bool bForce2D)
{
	if (NeighborVelocities.IsEmpty()) return FVector::ZeroVector;

	FVector AverageDirection = FVector::ZeroVector;

	for (const FVector& NeighborVel : NeighborVelocities)
	{
		FVector Vel = NeighborVel;
		if (bForce2D) Vel.Z = 0.0f;

		// Normalize before adding so fast neighbors don't dominate the average
		AverageDirection += Vel.GetSafeNormal();
	}

	AverageDirection /= NeighborVelocities.Num();

	// Convert the average heading into a desired velocity
	FVector DesiredVelocity = AverageDirection.GetSafeNormal() * MaxSpeed;
	if (bForce2D) DesiredVelocity.Z = 0.0f;

	return DesiredVelocity;
}

FVector UKzSteeringLibrary::Cohesion(const FVector& CurrentLocation, const FVector& CurrentVelocity, const TArray<FVector>& NeighborLocations, float MaxSpeed, bool bForce2D)
{
	if (NeighborLocations.IsEmpty()) return FVector::ZeroVector;

	// Include the agent's own position to calculate the true center of the flock
	FVector CenterOfMass = CurrentLocation;

	for (const FVector& NeighborLoc : NeighborLocations)
	{
		CenterOfMass += NeighborLoc;
	}

	// Divide by total flock size (neighbors + self)
	CenterOfMass /= (NeighborLocations.Num() + 1);

	// Seek towards the calculated center of mass
	return Seek(CurrentLocation, CenterOfMass, CurrentVelocity, MaxSpeed, bForce2D);
}

FVector UKzSteeringLibrary::CollisionAvoidance(const FVector& CurrentLocation, const FVector& CurrentVelocity, float AgentRadius, const TArray<FVector>& NeighborLocations, const TArray<FVector>& NeighborVelocities, float MaxLookAhead, float MaxSpeed, bool bForce2D)
{
	float CurrentSpeed = CurrentVelocity.Size();
	if (CurrentSpeed < UE_KINDA_SMALL_NUMBER) return FVector::ZeroVector;

	// Establish local space axes.
	FVector ForwardDir = CurrentVelocity / CurrentSpeed;
	FVector RightDir;

	if (bForce2D)
	{
		RightDir = FVector(-ForwardDir.Y, ForwardDir.X, 0.0f).GetSafeNormal();
	}
	else
	{
		RightDir = FVector::CrossProduct(FVector::UpVector, ForwardDir).GetSafeNormal();
		if (RightDir.IsNearlyZero())
		{
			RightDir = FVector::CrossProduct(FVector::ForwardVector, ForwardDir).GetSafeNormal();
		}
	}

	float FeelerLength = MaxLookAhead * MaxSpeed * (CurrentSpeed / MaxSpeed);
	FeelerLength = FMath::Max(FeelerLength, AgentRadius * 3.0f);

	int32 ThreatIndex = -1;
	float ClosestForwardDist = MAX_flt;
	float ThreatLateralDist = 0.0f;

	const float CombinedRadius = AgentRadius * 2.0f;
	const float AvoidanceRadius = CombinedRadius * 2.0f;

	// Find the most imminent threat inside the thick cylinder.
	for (int32 i = 0; i < NeighborLocations.Num(); ++i)
	{
		FVector ThreatLoc = NeighborLocations[i] + NeighborVelocities[i] * MaxLookAhead;
		if (bForce2D) ThreatLoc.Z = 0.0f;

		FVector ToThreat = ThreatLoc - CurrentLocation;

		// Project into local space.
		float ForwardDist = FVector::DotProduct(ToThreat, ForwardDir);
		float LateralDist = FVector::DotProduct(ToThreat, RightDir);

		float DistanceToFrontEdge = ForwardDist - AvoidanceRadius;

		// Check clearance buffer and ensure the front edge is within our feeler.
		if (ForwardDist > -AvoidanceRadius && DistanceToFrontEdge < FeelerLength)
		{
			if (FMath::Abs(LateralDist) < AvoidanceRadius)
			{
				if (ForwardDist < ClosestForwardDist)
				{
					ClosestForwardDist = ForwardDist;
					ThreatLateralDist = LateralDist;
					ThreatIndex = i;
				}
			}
		}
	}

	// Calculate smooth avoidance force.
	if (ThreatIndex >= 0)
	{
		FVector ThreatLoc = NeighborLocations[ThreatIndex];
		if (bForce2D) ThreatLoc.Z = 0.0f;

		if (FMath::IsNearlyZero(ThreatLateralDist))
		{
			ThreatLateralDist = 0.1f;
		}

		// Steer away from the threat's side.
		float SteerDirection = -FMath::Sign(ThreatLateralDist);

		float ForwardUrgency = 0.0f;
		float DistanceToFrontEdge = ClosestForwardDist - AvoidanceRadius;

		if (DistanceToFrontEdge > 0.0f)
		{
			// Approaching the obstacle's edge, urgency increases as distance closes.
			ForwardUrgency = 1.0f - (DistanceToFrontEdge / FeelerLength);
		}
		else if (ClosestForwardDist > 0.0f)
		{
			// Inside the front half of the obstacle, urgency is at maximum.
			ForwardUrgency = 1.0f;
		}
		else
		{
			// Passing the obstacle, urgency decreases as it gets left behind.
			ForwardUrgency = 1.0f - (FMath::Abs(ClosestForwardDist) / AvoidanceRadius);
		}

		ForwardUrgency = FMath::Clamp(ForwardUrgency, 0.0f, 1.0f);

		// Lateral urgency calculation.
		float LateralUrgency = 1.0f - (FMath::Abs(ThreatLateralDist) / AvoidanceRadius);
		LateralUrgency = FMath::Clamp(LateralUrgency, 0.0f, 1.0f);

		float TotalUrgency = ForwardUrgency * LateralUrgency;

		FVector AvoidanceDir = RightDir * SteerDirection;
		FVector DesiredVelocity = AvoidanceDir * CurrentSpeed * TotalUrgency;

		return DesiredVelocity;
	}

	return FVector::ZeroVector;
}