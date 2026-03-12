// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_Tether.h"
#include "GameFramework/Actor.h"

FVector UKzInputModifier_Tether::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	if (!Avatar || !AnchorSource.IsValid())
	{
		return CurrentInput;
	}

	FVector AvatarLoc = Avatar->GetActorLocation();
	FVector AnchorLoc = AnchorSource.GetLocation() + AnchorSource.GetVelocity() * 0.1f;

	// Vector from Anchor to Avatar on the XY plane
	FVector AnchorToAvatar = AvatarLoc - AnchorLoc;
	AnchorToAvatar.Z = 0.0f;

	float Dist = AnchorToAvatar.Size();
	if (Dist < UE_KINDA_SMALL_NUMBER)
	{
		return CurrentInput;
	}

	// Define our geometric axes relative to the circle
	FVector OutwardDir = AnchorToAvatar / Dist;

	// Decompose the player's intended input
	FVector TangentInput = FVector::VectorPlaneProject(CurrentInput, OutwardDir);
	float PlayerOutwardInput = FVector::DotProduct(CurrentInput, OutwardDir);

	// 1. Centripetal correction (We are currently outside the boundary)
	if (Dist > MaxDistance)
	{
		// Calculate how badly we have overshot
		float Overshoot = Dist - MaxDistance;

		// Calculate the required inward input to cancel the overshoot (Negative means pushing inward)
		float NeededInwardInput = -1.0f * FMath::Clamp(Overshoot / BrakingTolerance, 0.0f, 1.0f);

		// Combine intentions: We take the MOST inward value between what the player is doing and what the system needs.
		float FinalOutwardAmount = FMath::Min(PlayerOutwardInput, NeededInwardInput);

		// Reconstruct the final input vector
		FVector FinalInput = TangentInput + (OutwardDir * FinalOutwardAmount);

		// Clamp to ensure we don't accidentally create an input vector > 1.0 diagonally
		FinalInput = FinalInput.GetClampedToMaxSize(1.0f);
		FinalInput.Z = CurrentInput.Z;

		return FinalInput;
	}

	// 2. Tangent projection (Exactly at the edge, trying to push out)
	if (Dist >= (MaxDistance - UE_KINDA_SMALL_NUMBER) && PlayerOutwardInput > 0.0f)
	{
		TangentInput.Z = CurrentInput.Z;
		return TangentInput; // Allow pure orbital movement, delete the outward push
	}

	// 3. Safe Zone (Inside the circle)
	return CurrentInput;
}