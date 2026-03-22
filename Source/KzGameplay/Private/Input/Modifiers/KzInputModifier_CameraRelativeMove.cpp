// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_CameraRelativeMove.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

FVector UKzInputModifier_CameraRelativeMove::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	// We need a Pawn to get the Controller and its View Target.
	const APawn* Pawn = Cast<APawn>(Avatar);
	if (!Pawn || !Pawn->GetController())
	{
		// Fallback: Return the input unmodified if we cannot determine the camera orientation.
		return CurrentInput;
	}

	// Get the current view target (usually the camera associated with the player controller).
	AActor* ViewTarget = Pawn->GetController()->GetViewTarget();

	// Find out which way is forward based on the camera's rotation.
	const FRotator Rotation = ViewTarget ? ViewTarget->GetActorRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	// Get relative direction vectors from the pure Yaw rotation.
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Construct the World Space Vector using the current input's axes.
	// We map the Y-axis to Forward/Backward and the X-axis to Right/Left (Strafe).
	FVector WorldSpaceMove = (ForwardDirection * CurrentInput.Y) + (RightDirection * CurrentInput.X);

	// Preserve the original Z-axis input (Up/Down) just in case it is needed 
	// by other systems down the line (e.g., swimming or flying mechanics).
	WorldSpaceMove.Z = CurrentInput.Z;

	return WorldSpaceMove;
}