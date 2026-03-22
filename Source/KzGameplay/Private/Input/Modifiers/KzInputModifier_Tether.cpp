// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_Tether.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

FVector UKzInputModifier_Tether::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	if (!Avatar || !AnchorSource.IsValid())
	{
		return CurrentInput;
	}

	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		return CurrentInput;
	}

	const float DeltaTime = Avatar->GetActorTimeDilation() * World->GetDeltaSeconds();
	const FVector AnchorVelocity = AnchorSource.GetVelocity();
	const FVector AvatarVelocity = Avatar->GetVelocity();

	const float LookaheadTime = 0.2f;

	FVector AvatarLoc = Avatar->GetActorLocation() + (AvatarVelocity * LookaheadTime);
	FVector AnchorLoc = AnchorSource.GetLocation() + (AnchorVelocity * LookaheadTime);

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

	// Tangent projection
	if (Dist >= (MaxDistance - UE_KINDA_SMALL_NUMBER) && PlayerOutwardInput > 0.0f)
	{
		TangentInput.Z = CurrentInput.Z;
		return TangentInput; // Allow pure orbital movement, delete the outward push
	}

	// Safe Zone (Inside the circle)
	return CurrentInput;
}