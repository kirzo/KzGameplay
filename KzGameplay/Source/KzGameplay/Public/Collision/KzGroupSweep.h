// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"

class APawn;
class UPrimitiveComponent;
class UWorld;

/** A shape that moves with a group, swept from where it is now. */
struct KZGAMEPLAY_API FKzGroupSweepShape
{
	FCollisionShape Shape;
	FVector Start = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	ECollisionChannel Channel = ECC_WorldDynamic;
	FCollisionResponseParams Responses;

	/** A primitive's collision with its bottom raised by FloorClearance and its top where it was, so the floor it rests on never stops it. */
	static FKzGroupSweepShape FromPrimitive(const UPrimitiveComponent& Primitive, float FloorClearance = 4.0f);

	/** A pawn's collision with its bottom raised by the steps its movement climbs by itself, so a step it can take never stops the group. */
	static FKzGroupSweepShape FromPawn(const APawn& Pawn);
};

namespace KzGroupSweep
{
	/**
	 * How far a group of shapes can move by Delta together, in the horizontal plane, as one body: all of them are swept, the group stops at
	 * the earliest hit and slides what is left along it, up to MaxBounces times. It only falls short where no part of the move is free.
	 */
	KZGAMEPLAY_API FVector SlideHorizontal(const UWorld& World, TConstArrayView<FKzGroupSweepShape> Shapes, const FVector& Delta, const FCollisionQueryParams& Params, int32 MaxBounces = 3);
}