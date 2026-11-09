// Copyright 2026 kirzo

#include "Input/Modifiers/KzInputModifier_CameraRelativeMove.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

FVector UKzInputModifier_CameraRelativeMove::ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput)
{
	// Only a player has a camera to be relative to: anyone else, such as an AI, already moves in world space
	const APawn* Pawn = Cast<APawn>(Avatar);
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return CurrentInput;
	}

	// The real view rotation comes from the camera manager POV: it includes the
	// camera component / spring arm, which the view target's ACTOR rotation does not.
	const FRotator Rotation = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : PC->GetControlRotation();

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