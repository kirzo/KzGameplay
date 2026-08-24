// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_CameraRelativeDir.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

FVector UKzInputModifier_CameraRelativeDir::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
{
	const APawn* Pawn = Cast<APawn>(Avatar);
	if (!Pawn || !Pawn->GetController())
	{
		return CurrentInput;
	}

	// The real view rotation comes from the camera manager POV: it includes the
	// camera component / spring arm, which the view target's ACTOR rotation does not.
	float ViewTargetYaw = 0.0f;
	if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		ViewTargetYaw = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Yaw : PC->GetControlRotation().Yaw;
	}
	else if (const AActor* ViewTarget = Pawn->GetController()->GetViewTarget())
	{
		ViewTargetYaw = ViewTarget->GetActorRotation().Yaw;
	}

	// Map the raw input to a local 3D direction vector.
	// We swap X and Y here because standard gamepad mapping puts Y as forward/up and X as right.
	FVector LocalLookDirection = FVector(CurrentInput.Y, CurrentInput.X, 0.0f);

	// Rotate the local vector by the camera's Yaw to get the absolute World Direction
	FVector WorldLookDir = LocalLookDirection.RotateAngleAxis(ViewTargetYaw, FVector::UpVector);

	return WorldLookDir;
}