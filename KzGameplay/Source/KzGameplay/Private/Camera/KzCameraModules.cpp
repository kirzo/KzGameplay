// Copyright 2026 kirzo

#include "Camera/KzCameraModules.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"

UKzCameraModule_Lag::UKzCameraModule_Lag()
{
	Stage = EKzCameraStage::Rail;
	bHasState = true;
}

FKzCameraPose UKzCameraModule_Lag::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	if (!bInitialized || Context.bTeleport)
	{
		Distance = Pose.RailDistance;
		Rate = 0.0f;
		bInitialized = true;
	}
	else
	{
		FMath::CriticallyDampedSmoothing(Distance, Rate, Pose.RailDistance, 0.0f, Context.DeltaTime, LagTime);
	}

	FKzCameraPose Result = Pose;
	Result.RailDistance = Distance;
	return Result;
}

UKzCameraModule_SplineOffset::UKzCameraModule_SplineOffset()
{
	Stage = EKzCameraStage::Rail;
}

FKzCameraPose UKzCameraModule_SplineOffset::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FKzCameraPose Result = Pose;
	Result.RailDistance += Offset;

	const USplineComponent* Spline = Context.Follower ? Context.Follower->GetSpline() : nullptr;
	if (Spline && !Spline->IsClosedLoop())
	{
		Result.RailDistance = FMath::Clamp(Result.RailDistance, 0.0f, Spline->GetSplineLength());
	}

	return Result;
}

FKzCameraPose UKzCameraModule_PositionOffset::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FKzCameraPose Result = Pose;

	switch (Space)
	{
	case EKzCameraSpace::Reference:
		Result.Location += Context.Reference.TransformVectorNoScale(Offset);
		break;
	case EKzCameraSpace::Camera:
		Result.Location += Pose.Rotation.RotateVector(Offset);
		break;
	case EKzCameraSpace::World:
		Result.Location += Offset;
		break;
	}

	return Result;
}

UKzCameraModule_Pan::UKzCameraModule_Pan()
{
	bHasState = true;
}

FKzCameraPose UKzCameraModule_Pan::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FVector Desired = FVector::ZeroVector;

	if (const AActor* Target = Context.Target)
	{
		// How far the target is from its spline point, flattened onto the pan plane.
		Desired = Target->GetActorLocation() - Context.ProjectedLocation;

		const FVector Normal = (PlaneNormal.IsNearlyZero() ? Pose.Rotation.Vector() : Context.Reference.TransformVectorNoScale(PlaneNormal)).GetSafeNormal();
		Desired -= Normal * FVector::DotProduct(Desired, Normal);

		// Eases into MaxDistance instead of hitting it, so the camera slows down near the limit.
		const double Length = Desired.Size();
		if (MaxDistance > 0.0f && Length > UE_KINDA_SMALL_NUMBER)
		{
			Desired *= MaxDistance * FMath::Tanh(Length / MaxDistance) / Length;
		}
	}

	if (!bInitialized || Context.bTeleport)
	{
		Offset = Desired;
		Rate = FVector::ZeroVector;
		bInitialized = true;
	}
	else
	{
		FMath::CriticallyDampedSmoothing(Offset, Rate, Desired, FVector::ZeroVector, Context.DeltaTime, SmoothingTime);
	}

	FKzCameraPose Result = Pose;
	Result.Location += Offset;
	return Result;
}

UKzCameraModule_LookAt::UKzCameraModule_LookAt()
{
	Stage = EKzCameraStage::Aim;
}

FKzCameraPose UKzCameraModule_LookAt::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FKzCameraPose Result = Pose;

	if (Mode == EKzCameraLookAt::Direction)
	{
		if (!Direction.IsNearlyZero())
		{
			Result.Rotation = Direction.Rotation();
		}
		return Result;
	}

	FVector LookLocation;
	if (Mode == EKzCameraLookAt::Target)
	{
		if (!Context.Target) return Result;
		LookLocation = Context.Target->GetActorLocation() + TargetOffset;
	}
	else
	{
		if (!PointSource.IsValid())
		{
			// Without a pick the reference would fall back to the module's own component; the offset is a world location instead.
			const bool bPicked = Point.OverrideActor || !Point.ComponentName.IsNone();
			PointSource = bPicked ? Point.ToTransformSource(this) : FKzTransformSource(Point.GetRelativeTransform());
		}
		LookLocation = PointSource.GetLocation();
	}

	const FVector ToLook = LookLocation - Pose.Location;
	if (!ToLook.IsNearlyZero())
	{
		Result.Rotation = ToLook.Rotation();
	}

	return Result;
}

#if WITH_EDITOR
void UKzCameraModule_LookAt::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	PointSource.Invalidate();
}
#endif

UKzCameraModule_RotationOffset::UKzCameraModule_RotationOffset()
{
	Stage = EKzCameraStage::Rotation;
}

FKzCameraPose UKzCameraModule_RotationOffset::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FKzCameraPose Result = Pose;
	Result.Rotation = (Pose.Rotation.Quaternion() * Offset.Quaternion()).Rotator();
	return Result;
}

UKzCameraModule_FieldOfView::UKzCameraModule_FieldOfView()
{
	Stage = EKzCameraStage::Lens;
}

FKzCameraPose UKzCameraModule_FieldOfView::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FKzCameraPose Result = Pose;
	Result.FieldOfView = FieldOfView;
	return Result;
}

UKzCameraModule_DepthOfField::UKzCameraModule_DepthOfField()
{
	Stage = EKzCameraStage::Lens;
}

FKzCameraPose UKzCameraModule_DepthOfField::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FKzCameraPose Result = Pose;

	if (Focus == EKzCameraFocus::Distance)
	{
		Result.FocusDistance = FocusDistance;
	}
	else if (Context.Target)
	{
		Result.FocusDistance = FVector::Dist(Pose.Location, Context.Target->GetActorLocation());
	}
	else
	{
		return Result;
	}

	Result.Aperture = Aperture;
	return Result;
}
