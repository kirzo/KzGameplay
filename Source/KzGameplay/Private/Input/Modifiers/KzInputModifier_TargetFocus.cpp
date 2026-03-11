// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_TargetFocus.h"
#include "GameFramework/Actor.h"

FVector UKzInputModifier_TargetFocus::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	if (!Avatar || !TargetSource.IsValid())
	{
		return CurrentInput;
	}

	FVector AvatarLoc = Avatar->GetActorLocation();
	FVector TargetLoc = TargetSource.GetLocation();

	// We only care about the XY plane for this focus calculation
	FVector ToTarget = TargetLoc - AvatarLoc;
	ToTarget.Z = 0.0f;

	if (ToTarget.IsNearlyZero())
	{
		return CurrentInput;
	}

	FVector ToTargetDir = ToTarget.GetSafeNormal();

	// 1. Handle the "Hands Off" state
	if (CurrentInput.IsNearlyZero())
	{
		if (bForceFocusOnZeroInput)
		{
			// Inject a normalized vector pointing directly at the target so the Avatar auto-turns
			return ToTargetDir;
		}
		return CurrentInput;
	}

	// 2. Decompose the current input
	FVector InputDir = CurrentInput;
	InputDir.Z = 0.0f;
	float InputMag = InputDir.Size();

	if (InputMag < UE_KINDA_SMALL_NUMBER)
	{
		return CurrentInput;
	}

	InputDir /= InputMag; // Normalize

	// 3. Calculate the angle deviation using the Dot Product
	float Dot = FVector::DotProduct(ToTargetDir, InputDir);
	Dot = FMath::Clamp(Dot, -1.0f, 1.0f); // Protect against floating point inaccuracy for Acos

	float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// 4. If the input is within the allowed cone, let it pass unchanged
	if (AngleDegrees <= ConeHalfAngle)
	{
		return CurrentInput;
	}

	// 5. Determine which side (Left or Right) the input lies on using the 2D Cross Product.
	float Cross2D = (ToTargetDir.X * InputDir.Y) - (ToTargetDir.Y * InputDir.X);
	float Sign = FMath::Sign(Cross2D);

	// Rotate the pure target vector towards the user's input by the maximum allowed angle
	FVector ClampedDir = ToTargetDir.RotateAngleAxis(ConeHalfAngle * Sign, FVector::UpVector);

	// Reconstruct the final vector maintaining the original magnitude and Z-axis
	FVector FinalInput = ClampedDir * InputMag;
	FinalInput.Z = CurrentInput.Z;

	return FinalInput;
}