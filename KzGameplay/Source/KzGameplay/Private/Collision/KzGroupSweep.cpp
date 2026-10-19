// Copyright 2026 kirzo

#include "Collision/KzGroupSweep.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

FKzGroupSweepShape FKzGroupSweepShape::FromPrimitive(const UPrimitiveComponent& Primitive, float FloorClearance)
{
	// Half the clearance lifts the shape and half shortens it, so the bottom rises by all of it and the top stays
	const double Half = 0.5 * FloorClearance;

	FKzGroupSweepShape Result;
	Result.Start = Primitive.Bounds.Origin + FVector(0.0, 0.0, Half);
	Result.Rotation = Primitive.GetComponentQuat();
	Result.Channel = Primitive.GetCollisionObjectType();
	Result.Responses = FCollisionResponseParams(Primitive.GetCollisionResponseToChannels());
	Result.Shape = Primitive.GetCollisionShape();

	if (Result.Shape.IsBox())
	{
		// The box of the primitive's own bounds, turned with it, rather than the world-aligned one
		FVector Extent = Primitive.CalcLocalBounds().BoxExtent * Primitive.GetComponentScale().GetAbs();
		Extent.Z = FMath::Max(1.0, Extent.Z - Half);
		Result.Shape.SetBox(FVector3f(Extent));
	}
	else if (Result.Shape.IsCapsule())
	{
		const float Radius = Result.Shape.GetCapsuleRadius();
		Result.Shape.SetCapsule(Radius, FMath::Max(Radius, Result.Shape.GetCapsuleHalfHeight() - static_cast<float>(Half)));
	}

	return Result;
}

FKzGroupSweepShape FKzGroupSweepShape::FromPawn(const APawn& Pawn)
{
	const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Pawn.GetRootComponent());
	if (!Root)
	{
		FKzGroupSweepShape Point;
		Point.Start = Pawn.GetActorLocation();
		Point.Shape = FCollisionShape::MakeSphere(0.0f);
		return Point;
	}

	const UCharacterMovementComponent* Movement = Cast<UCharacterMovementComponent>(Pawn.GetMovementComponent());
	return FromPrimitive(*Root, Movement ? Movement->MaxStepHeight : 4.0f);
}

FVector KzGroupSweep::SlideHorizontal(const UWorld& World, TConstArrayView<FKzGroupSweepShape> Shapes, const FVector& Delta, const FCollisionQueryParams& Params, int32 MaxBounces)
{
	// Stops this far short of a hit, so the next sweep does not start touching it
	constexpr double Skin = 0.1;

	FVector Moved = FVector::ZeroVector;
	FVector Remaining(Delta.X, Delta.Y, 0.0);
	for (int32 Bounce = 0; Bounce < MaxBounces && !Remaining.IsNearlyZero(); ++Bounce)
	{
		// The earliest hit of any shape stops the whole group
		FHitResult Earliest;
		bool bHit = false;
		for (const FKzGroupSweepShape& Shape : Shapes)
		{
			const FVector Start = Shape.Start + Moved;
			FHitResult Hit;
			if (!World.SweepSingleByChannel(Hit, Start, Start + Remaining, Shape.Rotation, Shape.Channel, Shape.Shape, Params, Shape.Responses) || !Hit.bBlockingHit)
			{
				continue;
			}

			// Already touching something it moves away from: not in the way
			if (Hit.bStartPenetrating && (FVector(Hit.Normal) | Remaining) >= 0.0)
			{
				continue;
			}

			if (!bHit || Hit.Time < Earliest.Time)
			{
				Earliest = Hit;
				bHit = true;
			}
		}

		if (!bHit)
		{
			Moved += Remaining;
			break;
		}

		const double SafeTime = FMath::Max(0.0, Earliest.Time - Skin / Remaining.Size());
		Moved += Remaining * SafeTime;

		// What is left slides along the hit; one with no side to it, such as a low ceiling, leaves nothing to slide along
		FVector Normal(Earliest.Normal.X, Earliest.Normal.Y, 0.0);
		if (!Normal.Normalize())
		{
			break;
		}

		const FVector Unsafe = Remaining * (1.0 - SafeTime);
		Remaining = (Unsafe | Normal) < 0.0 ? FVector::VectorPlaneProject(Unsafe, Normal) : Unsafe;
		Remaining.Z = 0.0;
	}

	return Moved;
}

double KzGroupSweep::RotateYaw(const UWorld& World, TArrayView<FKzGroupSweepShape> Shapes, const FVector& Pivot, double Yaw, const FCollisionQueryParams& Params, int32 MaxHalvings)
{
	// What the group already touches does not count, so it can turn away from a wall it rests against.
	TSet<const UPrimitiveComponent*> Touching;
	TArray<FOverlapResult> Overlaps;
	for (const FKzGroupSweepShape& Shape : Shapes)
	{
		Overlaps.Reset();
		World.OverlapMultiByChannel(Overlaps, Shape.Start, Shape.Rotation, Shape.Channel, Shape.Shape, Params, Shape.Responses);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.bBlockingHit)
			{
				Touching.Add(Overlap.GetComponent());
			}
		}
	}

	auto RunsIntoSomething = [&](const FQuat& Turn)
	{
		for (const FKzGroupSweepShape& Shape : Shapes)
		{
			Overlaps.Reset();
			World.OverlapMultiByChannel(Overlaps, Pivot + Turn.RotateVector(Shape.Start - Pivot), Turn * Shape.Rotation, Shape.Channel, Shape.Shape, Params, Shape.Responses);
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (Overlap.bBlockingHit && !Touching.Contains(Overlap.GetComponent()))
				{
					return true;
				}
			}
		}
		return false;
	};

	for (int32 Halving = 0; Halving <= MaxHalvings; ++Halving, Yaw *= 0.5)
	{
		const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(Yaw));
		if (RunsIntoSomething(Turn))
		{
			continue;
		}

		for (FKzGroupSweepShape& Shape : Shapes)
		{
			Shape.Start = Pivot + Turn.RotateVector(Shape.Start - Pivot);
			Shape.Rotation = Turn * Shape.Rotation;
		}
		return Yaw;
	}

	return 0.0;
}