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

namespace
{
	/** Whether something solid holds a body that simulates physics against a push by Delta, so the push would crush it rather than move it. */
	bool IsPinned(const UWorld& World, UPrimitiveComponent& Body, const FVector& Delta, FCollisionQueryParams Params)
	{
		Params.AddIgnoredComponent(&Body);

		const FKzGroupSweepShape Shape = FKzGroupSweepShape::FromPrimitive(Body);
		FHitResult Hit;
		if (!World.SweepSingleByChannel(Hit, Shape.Start, Shape.Start + Delta, Shape.Rotation, Shape.Channel, Shape.Shape, Params, Shape.Responses) || !Hit.bBlockingHit)
		{
			return false;
		}

		// ponytail: one level deep, another body that simulates counts as free; a chain of them can still be crushed against a wall
		const UPrimitiveComponent* Other = Hit.GetComponent();
		const bool bMovingAway = Hit.bStartPenetrating && (FVector(Hit.Normal) | Delta) >= 0.0;
		return !bMovingAway && !(Other && Other->IsSimulatingPhysics());
	}

	/** The first hit that stops Shape on its way by Delta. With OutPushed, sees through the bodies that simulate physics and are free to move, collecting and ignoring them. */
	bool SweepToBlock(const UWorld& World, const FKzGroupSweepShape& Shape, const FVector& Start, const FVector& Delta, FCollisionQueryParams& Params, TArray<UPrimitiveComponent*>* OutPushed, FHitResult& OutHit)
	{
		// Bounded, each pass sees through one more body
		for (int32 Pass = 0; Pass < 8; ++Pass)
		{
			if (!World.SweepSingleByChannel(OutHit, Start, Start + Delta, Shape.Rotation, Shape.Channel, Shape.Shape, Params, Shape.Responses) || !OutHit.bBlockingHit)
			{
				return false;
			}

			UPrimitiveComponent* Body = OutHit.GetComponent();
			if (!OutPushed || !Body || !Body->IsSimulatingPhysics() || IsPinned(World, *Body, Delta, Params))
			{
				return true;
			}

			OutPushed->AddUnique(Body);
			Params.AddIgnoredComponent(Body);
		}

		return true;
	}
}

FVector KzGroupSweep::SlideHorizontal(const UWorld& World, TConstArrayView<FKzGroupSweepShape> Shapes, const FVector& Delta, const FCollisionQueryParams& Params, TArray<UPrimitiveComponent*>* OutPushed, int32 MaxBounces)
{
	// Stops this far short of a hit, so the next sweep does not start touching it
	constexpr double Skin = 0.1;

	// Bodies found free to push stay ignored for the rest of the group's sweeps
	FCollisionQueryParams QueryParams = Params;

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
			if (!SweepToBlock(World, Shape, Start, Remaining, QueryParams, OutPushed, Hit))
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

double KzGroupSweep::RotateYaw(const UWorld& World, TArrayView<FKzGroupSweepShape> Shapes, const FVector& Pivot, double Yaw, const FCollisionQueryParams& Params, TArray<UPrimitiveComponent*>* OutPushed, int32 MaxHalvings)
{
	// ponytail: no pinned check here, a turn small enough per frame rarely crushes anything; SlideHorizontal's IsPinned if it does
	auto Blocks = [OutPushed](const FOverlapResult& Overlap)
	{
		UPrimitiveComponent* Component = Overlap.GetComponent();
		if (OutPushed && Component && Component->IsSimulatingPhysics())
		{
			OutPushed->AddUnique(Component);
			return false;
		}
		return static_cast<bool>(Overlap.bBlockingHit);
	};

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
				if (Blocks(Overlap) && !Touching.Contains(Overlap.GetComponent()))
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