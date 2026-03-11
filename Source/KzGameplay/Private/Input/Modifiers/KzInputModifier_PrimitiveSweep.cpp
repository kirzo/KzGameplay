// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_PrimitiveSweep.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

FVector UKzInputModifier_PrimitiveSweep::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	if (!Avatar || !SweepPrimitive || CurrentInput.IsNearlyZero())
	{
		return CurrentInput;
	}

	UWorld* World = Avatar->GetWorld();
	if (!World) return CurrentInput;

	// 1. Setup Collision Params
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	if (const AActor* PrimitiveOwner = SweepPrimitive->GetOwner())
	{
		QueryParams.AddIgnoredActor(PrimitiveOwner);
	}
	else
	{
		QueryParams.AddIgnoredComponent(SweepPrimitive.Get());
	}

	// Ignore everything attached to the avatar
	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors, true);
	QueryParams.AddIgnoredActors(AttachedActors);
	QueryParams.AddIgnoredActors(IgnoredActors);

	// 2. Define the Sweep
	FVector StartLoc = SweepPrimitive->GetComponentLocation();
	FQuat SweepQuat = SweepPrimitive->GetComponentQuat();
	FCollisionShape Shape = SweepPrimitive->GetCollisionShape();

	// We sweep AS the primitive's object type (e.g., PhysicsBody, WorldDynamic)
	ECollisionChannel ObjectType = SweepPrimitive->GetCollisionObjectType();

	// We extract its exact response matrix (what it blocks, overlaps, or ignores)
	FCollisionResponseParams ResponseParams(SweepPrimitive->GetCollisionResponseToChannels());

	// 3. Define the Sweep
	FVector MoveDirection = CurrentInput.GetSafeNormal();
	FVector EndLoc = StartLoc + (MoveDirection * SweepDistance);

	FHitResult Hit;
	bool bHit = World->SweepSingleByChannel(Hit, StartLoc, EndLoc, SweepQuat, ObjectType, Shape, QueryParams, ResponseParams);

	// 3. Process Result
	if (bHit && Hit.bBlockingHit)
	{
		// Calculate the "Slide" vector. 
		// If we are moving towards a wall, project the input onto the wall's plane.
		float DotToWall = FVector::DotProduct(CurrentInput, Hit.Normal);

		// Only modify if we are actually pushing INTO the wall
		if (DotToWall < 0.0f)
		{
			FVector SlidInput = FVector::VectorPlaneProject(CurrentInput, Hit.Normal);

			// Maintain the original vertical input if necessary
			SlidInput.Z = CurrentInput.Z;

			return SlidInput;
		}
	}

	return CurrentInput;
}