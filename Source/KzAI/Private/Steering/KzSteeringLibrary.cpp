// Copyright 2026 kirzo

#include "Steering/KzSteeringLibrary.h"

FVector UKzSteeringLibrary::Seek(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, bool bForce2D)
{
	FVector ToTarget = TargetLocation - CurrentLocation;
	if (bForce2D) ToTarget.Z = 0.0f;

	const FVector DesiredVelocity = ToTarget.GetSafeNormal() * MaxSpeed;

	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	if (bForce2D) SteeringForce.Z = 0.0f;

	return SteeringForce;
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

	const FVector DesiredVelocity = (ToTarget / Distance) * ClippedSpeed;

	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	if (bForce2D) SteeringForce.Z = 0.0f;

	return SteeringForce;
}

FVector UKzSteeringLibrary::Flee(const FVector& CurrentLocation, const FVector& TargetLocation, const FVector& CurrentVelocity, float MaxSpeed, bool bForce2D)
{
	FVector AwayFromTarget = CurrentLocation - TargetLocation;
	if (bForce2D) AwayFromTarget.Z = 0.0f;

	const FVector DesiredVelocity = AwayFromTarget.GetSafeNormal() * MaxSpeed;

	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	if (bForce2D) SteeringForce.Z = 0.0f;

	return SteeringForce;
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

FVector UKzSteeringLibrary::Separation(const FVector& CurrentLocation, const FVector& CurrentVelocity, const TArray<FVector>& NeighborLocations, float MaxSpeed, bool bForce2D)
{
	FVector PushForce = FVector::ZeroVector;
	int32 Count = 0;

	for (const FVector& NeighborLoc : NeighborLocations)
	{
		FVector ToAgent = CurrentLocation - NeighborLoc;
		if (bForce2D) ToAgent.Z = 0.0f;

		const float Distance = ToAgent.Size();
		if (Distance > 0.0f)
		{
			// Force is inversely proportional to distance (closer = pushes harder)
			PushForce += (ToAgent.GetSafeNormal() / Distance);
			Count++;
		}
	}

	if (Count == 0) return FVector::ZeroVector;

	FVector DesiredVelocity = PushForce.GetSafeNormal() * MaxSpeed;
	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	if (bForce2D) SteeringForce.Z = 0.0f;

	return SteeringForce;
}

FVector UKzSteeringLibrary::Alignment(const FVector& CurrentVelocity, const TArray<FVector>& NeighborVelocities, float MaxSpeed, bool bForce2D)
{
	FVector AverageVelocity = FVector::ZeroVector;

	for (const FVector& NeighborVel : NeighborVelocities)
	{
		AverageVelocity += NeighborVel;
	}

	if (NeighborVelocities.Num() > 0)
	{
		AverageVelocity /= NeighborVelocities.Num();
	}
	else
	{
		return FVector::ZeroVector;
	}

	if (bForce2D) AverageVelocity.Z = 0.0f;

	FVector DesiredVelocity = AverageVelocity.GetSafeNormal() * MaxSpeed;
	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	if (bForce2D) SteeringForce.Z = 0.0f;

	return SteeringForce;
}

FVector UKzSteeringLibrary::Cohesion(const FVector& CurrentLocation, const FVector& CurrentVelocity, const TArray<FVector>& NeighborLocations, float MaxSpeed, bool bForce2D)
{
	FVector CenterOfMass = FVector::ZeroVector;

	for (const FVector& NeighborLoc : NeighborLocations)
	{
		CenterOfMass += NeighborLoc;
	}

	if (NeighborLocations.Num() > 0)
	{
		CenterOfMass /= NeighborLocations.Num();
		return Seek(CurrentLocation, CenterOfMass, CurrentVelocity, MaxSpeed, bForce2D);
	}

	return FVector::ZeroVector;
}