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
	FVector StartLoc = SweepPrimitive->Bounds.Origin + SweepPrimitive->ComponentVelocity * 0.1f;
	FQuat SweepQuat = SweepPrimitive->GetComponentQuat();
	FCollisionShape Shape = SweepPrimitive->GetCollisionShape();

	// --- Floor Scraping Prevention ---
	// Lift the sweep slightly and shrink the vertical size so we don't catch the floor
	const float FloorClearance = 2.0f;
	StartLoc.Z += FloorClearance;

	if (Shape.IsBox())
	{
		FBoxSphereBounds LocalBounds = SweepPrimitive->CalcLocalBounds();

		FVector Extent = LocalBounds.BoxExtent;
		// Shrink Z extent to compensate for the lift, keeping the top at the same height
		Extent.Z = FMath::Max(1.0f, Extent.Z - FloorClearance);
		Shape.SetBox(FVector3f(Extent));
	}
	else if (Shape.IsCapsule())
	{
		float Radius = Shape.GetCapsuleRadius();
		float HalfHeight = FMath::Max(Radius, Shape.GetCapsuleHalfHeight() - FloorClearance);
		Shape.SetCapsule(Radius, HalfHeight);
	}

	ECollisionChannel ObjectType = SweepPrimitive->GetCollisionObjectType();
	FCollisionResponseParams ResponseParams(SweepPrimitive->GetCollisionResponseToChannels());

	// 3. Multi-Bounce Slide Logic
	FVector RemainingInput = CurrentInput;
	FVector AccumulatedInput = FVector::ZeroVector;

	const int32 MaxBounces = 3;

	for (int32 Bounce = 0; Bounce < MaxBounces; ++Bounce)
	{
		if (RemainingInput.IsNearlyZero())
		{
			break;
		}

		// Calculate how far this remaining input wants to push us
		FVector MoveDelta = RemainingInput * SweepDistance;
		FVector EndLoc = StartLoc + MoveDelta;

		FHitResult Hit;
		bool bHit = World->SweepSingleByChannel(Hit, StartLoc, EndLoc, SweepQuat, ObjectType, Shape, QueryParams, ResponseParams);

		if (bHit && Hit.bBlockingHit)
		{
			// Calculate the percentage of input used before the impact
			float SafeTime = FMath::Max(0.0f, Hit.Time - UE_KINDA_SMALL_NUMBER);

			// 1. Keep the safe portion of the input
			FVector SafeInput = RemainingInput * SafeTime;
			AccumulatedInput += SafeInput;

			// 2. The unsafe portion is what would have penetrated the obstacle
			FVector UnsafeInput = RemainingInput * (1.0f - SafeTime);

			FVector Normal2D = Hit.Normal;
			Normal2D.Z = 0.0f;

			// Prevent NaNs if the hit normal was completely vertical (e.g., floor/ceiling)
			if (Normal2D.IsNearlyZero())
			{
				RemainingInput = UnsafeInput;
				Normal2D = RemainingInput.GetSafeNormal2D(); // Fallback direction for the nudge
			}
			else
			{
				Normal2D.Normalize();

				// Only slide if we are pushing INTO the wall
				if (FVector::DotProduct(UnsafeInput, Normal2D) < 0.0f)
				{
					RemainingInput = FVector::VectorPlaneProject(UnsafeInput, Normal2D);
				}
				else
				{
					RemainingInput = UnsafeInput;
				}
			}

			RemainingInput.Z = CurrentInput.Z;

			// 3. Update the start location for the next bounce sweep.
			if (Hit.bStartPenetrating)
			{
				// We started the sweep already overlapping an obstacle.
				// Extract the StartLoc by the EXACT penetration depth plus a safe margin.
				StartLoc += Normal2D * (Hit.PenetrationDepth + 0.1f);
			}
			else
			{
				// Standard advance to the impact point plus a tiny nudge.
				StartLoc = StartLoc + (MoveDelta * SafeTime) + (Normal2D * 0.1f);
			}
		}
		else
		{
			// No hit, the remaining input is completely safe
			AccumulatedInput += RemainingInput;
			break; // We reached our destination freely
		}
	}

	// 4. Restore original vertical input if you want to allow gravity/jumping to bypass wall sliding logic
	AccumulatedInput.Z = CurrentInput.Z;

	return AccumulatedInput;
}