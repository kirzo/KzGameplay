// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_CameraRelativeDir.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

FVector UKzInputModifier_CameraRelativeDir::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	const APawn* Pawn = Cast<APawn>(Avatar);
	if (!Pawn || !Pawn->GetController())
	{
		return CurrentInput;
	}

	// Get the current view target (usually the camera)
	AActor* ViewTarget = Pawn->GetController()->GetViewTarget();
	const float ViewTargetYaw = ViewTarget ? ViewTarget->GetActorRotation().Yaw : 0.0f;

	// Map the raw input to a local 3D direction vector.
	// We swap X and Y here because standard gamepad mapping puts Y as forward/up and X as right.
	FVector LocalLookDirection = FVector(CurrentInput.Y, CurrentInput.X, 0.0f);

	// Rotate the local vector by the camera's Yaw to get the absolute World Direction
	FVector WorldLookDir = LocalLookDirection.RotateAngleAxis(ViewTargetYaw, FVector::UpVector);

	return WorldLookDir;
}