// Copyright 2026 kirzo

#include "Camera/KzCameraTypes.h"

FKzCameraPose FKzCameraPose::Blend(const FKzCameraPose& A, const FKzCameraPose& B, float Alpha)
{
	constexpr float UnfocusedAperture = 22.0f;

	FKzCameraPose Result;
	Result.Focus = FMath::Lerp(A.Focus, B.Focus, Alpha);
	Result.FocusSlack = FMath::Lerp(A.FocusSlack, B.FocusSlack, Alpha);
	Result.Location = FMath::Lerp(A.Location, B.Location, Alpha);
	Result.Rotation = FQuat::Slerp(A.Rotation.Quaternion(), B.Rotation.Quaternion(), Alpha).Rotator();
	Result.FieldOfView = FMath::Lerp(A.FieldOfView, B.FieldOfView, Alpha);

	const bool bFocusA = A.FocusDistance > 0.0f;
	const bool bFocusB = B.FocusDistance > 0.0f;
	if (bFocusA || bFocusB)
	{
		Result.FocusDistance = FMath::Lerp(bFocusA ? A.FocusDistance : B.FocusDistance, bFocusB ? B.FocusDistance : A.FocusDistance, Alpha);
		Result.Aperture = FMath::Lerp(bFocusA ? A.Aperture : UnfocusedAperture, bFocusB ? B.Aperture : UnfocusedAperture, Alpha);
	}

	return Result;
}
