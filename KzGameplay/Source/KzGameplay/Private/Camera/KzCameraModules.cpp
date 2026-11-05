// Copyright 2026 kirzo

#include "Camera/KzCameraModules.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

UKzCameraModule_DeadZone::UKzCameraModule_DeadZone()
{
	Stage = EKzCameraStage::Track;
	bHasState = true;
}

FVector UKzCameraModule_DeadZone::GetHalfSize(const FKzCameraContext& Context) const
{
	if (Units == EKzCameraDeadZoneUnits::Centimeters)
	{
		return FVector(Depth, Horizontal, Vertical);
	}

	// Half the view, at the depth of the focus.
	const FMinimalViewInfo& View = Context.LastView;
	const double FocusDepth = FMath::Max(FVector::DotProduct(Center - View.Location, View.Rotation.Vector()), 1.0);
	const double HalfWidth = FocusDepth * FMath::Tan(FMath::DegreesToRadians(View.FOV * 0.5));
	const double HalfHeight = HalfWidth / FMath::Max(View.AspectRatio, 0.01f);
	return FVector(Depth, HalfWidth * Horizontal, HalfHeight * Vertical);
}

FKzCameraPose UKzCameraModule_DeadZone::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	HalfSize = GetHalfSize(Context);

	if (!bInitialized || Context.bTeleport)
	{
		Center = Pose.Focus;
		bInitialized = true;
	}
	else
	{
		// Along each view axis, whatever sticks out of the box drags it.
		const FQuat Axes = Context.Rest.Quaternion();
		const FVector Offset = Axes.UnrotateVector(Pose.Focus - Center);

		FVector Drag = FVector::ZeroVector;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const double Excess = FMath::Abs(Offset[Axis]) - HalfSize[Axis];
			if (Excess > 0.0)
			{
				Drag[Axis] = FMath::Sign(Offset[Axis]) * Excess;
			}
		}

		Center += Axes.RotateVector(Drag);
	}

	FKzCameraPose Result = Pose;
	Result.Focus = Center;
	return Result;
}

void UKzCameraModule_DeadZone::DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const
{
	DrawDebugBox(GetWorld(), Center, HalfSize.ComponentMax(FVector(1.0)), Context.Rest.Quaternion(), FColor::Cyan);
}

UKzCameraModule_LookAhead::UKzCameraModule_LookAhead()
{
	Stage = EKzCameraStage::Lead;
	bHasState = true;
}

FKzCameraPose UKzCameraModule_LookAhead::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	FVector Velocity = FVector::ZeroVector;

	if (const AActor* Target = Context.Target)
	{
		const APawn* Pawn = Cast<APawn>(Target);
		const UPawnMovementComponent* Movement = Pawn ? Pawn->GetMovementComponent() : nullptr;

		if (Source == EKzCameraLeadSource::MovementInput && Movement)
		{
			Velocity = Pawn->GetLastMovementInputVector().GetClampedToMaxSize(1.0) * Movement->GetMaxSpeed();
		}
		else
		{
			Velocity = Target->GetVelocity();
		}
	}

	// Only across the screen: a lead in depth would push the camera into or away from the target.
	const FVector Forward = Context.Rest.Vector();
	Velocity -= Forward * FVector::DotProduct(Velocity, Forward);

	// Standing still keeps the last lead, so the camera keeps showing where the target was heading.
	const bool bIdle = Velocity.IsNearlyZero(1.0);
	if (!bHoldWhenIdle || !bIdle || !bInitialized || Context.bTeleport)
	{
		DesiredLead = (Velocity * LeadTime).GetClampedToMaxSize(MaxLead);
	}

	if (!bInitialized || Context.bTeleport)
	{
		Lead = DesiredLead;
		Rate = FVector::ZeroVector;
		bInitialized = true;
	}
	else
	{
		FMath::CriticallyDampedSmoothing(Lead, Rate, DesiredLead, FVector::ZeroVector, Context.DeltaTime, SmoothingTime);
	}

	FKzCameraPose Result = Pose;
	Result.Focus += Lead;
	return Result;
}

void UKzCameraModule_LookAhead::DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const
{
	if (!Lead.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(GetWorld(), Context.TargetLocation, Context.TargetLocation + Lead, 30.0f, FColor::Green);
	}
}

UKzCameraModule_Lag::UKzCameraModule_Lag()
{
	Stage = EKzCameraStage::Smooth;
	bHasState = true;
}

FKzCameraPose UKzCameraModule_Lag::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	const float DeltaTime = Context.DeltaTime;

	if (!bInitialized || Context.bTeleport)
	{
		Position = Pose.Focus;
		Velocity = FVector::ZeroVector;
		bInitialized = true;
	}
	else if (DeltaTime > UE_KINDA_SMALL_NUMBER)
	{
		FVector Smoothed = Position;
		FVector SmoothedVelocity = Velocity;
		FMath::CriticallyDampedSmoothing(Smoothed, SmoothedVelocity, Pose.Focus, FVector::ZeroVector, DeltaTime, LagTime);

		if (MaxSpeed <= 0.0f && MaxAcceleration <= 0.0f)
		{
			Position = Smoothed;
			Velocity = SmoothedVelocity;
		}
		else
		{
			// The smoothing says where to go; the caps say how fast it may get there.
			FVector NewVelocity = (Smoothed - Position) / DeltaTime;

			if (MaxAcceleration > 0.0f)
			{
				NewVelocity = Velocity + (NewVelocity - Velocity).GetClampedToMaxSize(MaxAcceleration * DeltaTime);

				// Never faster than what can still stop on the target.
				NewVelocity = NewVelocity.GetClampedToMaxSize(FMath::Sqrt(2.0 * MaxAcceleration * FVector::Dist(Position, Pose.Focus)));
			}

			if (MaxSpeed > 0.0f)
			{
				NewVelocity = NewVelocity.GetClampedToMaxSize(MaxSpeed);
			}

			Velocity = NewVelocity;
			Position += Velocity * DeltaTime;
		}
	}

	FKzCameraPose Result = Pose;
	Result.Focus = Position;
	return Result;
}

UKzCameraModule_SplineOffset::UKzCameraModule_SplineOffset()
{
	Stage = EKzCameraStage::Anchor;
}

FKzCameraPose UKzCameraModule_SplineOffset::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	const UKzSplineFollowerComponent* SplineFollower = Context.Follower;
	const USplineComponent* Spline = SplineFollower ? SplineFollower->GetSpline() : nullptr;
	if (!Spline) return Pose;

	float Distance = SplineFollower->GetCurrentDistance() + Offset;
	if (!Spline->IsClosedLoop())
	{
		Distance = FMath::Clamp(Distance, 0.0f, Spline->GetSplineLength());
	}

	const FTransform CameraTransform = Context.CameraRelative * SplineFollower->GetTransformAtDistance(Distance);

	FKzCameraPose Result = Pose;
	Result.Location = CameraTransform.GetLocation();
	Result.Rotation = CameraTransform.Rotator();
	return Result;
}

UKzCameraModule_Follow::UKzCameraModule_Follow()
{
	Stage = EKzCameraStage::Anchor;
}

FKzCameraPose UKzCameraModule_Follow::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	const FVector AnchorLocation = Context.Anchor.GetLocation();

	FVector HangingPoint = Pose.Focus;
	if (bLockX) HangingPoint.X = AnchorLocation.X;
	if (bLockY) HangingPoint.Y = AnchorLocation.Y;
	if (bLockZ) HangingPoint.Z = AnchorLocation.Z;

	const FTransform CameraTransform = Context.CameraRelative * FTransform(Context.Anchor.GetRotation(), HangingPoint);

	FKzCameraPose Result = Pose;
	Result.Location = CameraTransform.GetLocation();
	Result.Rotation = CameraTransform.Rotator();
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

FVector UKzCameraModule_Pan::GetPlaneNormal(const FKzCameraContext& Context, const FKzCameraPose& Pose) const
{
	return (PlaneNormal.IsNearlyZero() ? Pose.Rotation.Vector() : Context.Reference.TransformVectorNoScale(PlaneNormal)).GetSafeNormal();
}

FKzCameraPose UKzCameraModule_Pan::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	// How far the focus is from where the camera hangs, flattened onto the pan plane.
	const FVector Normal = GetPlaneNormal(Context, Pose);
	FVector Pan = Pose.Focus - Context.Anchor.GetLocation();
	Pan -= Normal * FVector::DotProduct(Pan, Normal);

	// Eases into MaxDistance instead of hitting it, so the camera slows down near the limit.
	const double Length = Pan.Size();
	if (MaxDistance > 0.0f && Length > UE_KINDA_SMALL_NUMBER)
	{
		Pan *= MaxDistance * FMath::Tanh(Length / MaxDistance) / Length;
	}

	FKzCameraPose Result = Pose;
	Result.Location += Pan;
	return Result;
}

void UKzCameraModule_Pan::DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const
{
	if (MaxDistance <= 0.0f) return;

	FVector AxisY;
	FVector AxisZ;
	GetPlaneNormal(Context, Pose).FindBestAxisVectors(AxisY, AxisZ);
	DrawDebugCircle(GetWorld(), Context.Anchor.GetLocation(), MaxDistance, 32, FColor::Orange, false, -1.0f, 0, 0.0f, AxisY, AxisZ, false);
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
		LookLocation = Context.TargetLocation + TargetOffset;
	}
	else if (Mode == EKzCameraLookAt::Focus)
	{
		LookLocation = Pose.Focus + TargetOffset;
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

UKzCameraModule_RotationLimit::UKzCameraModule_RotationLimit()
{
	Stage = EKzCameraStage::Constraint;
}

FQuat UKzCameraModule_RotationLimit::GetBase(const FKzCameraContext& Context) const
{
	if (Base == EKzCameraRotationBase::World && !Direction.IsNearlyZero())
	{
		return Direction.Rotation().Quaternion();
	}
	return Context.Rest.Quaternion();
}

FKzCameraPose UKzCameraModule_RotationLimit::Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose)
{
	const FQuat BaseRotation = GetBase(Context);
	const FVector Local = BaseRotation.UnrotateVector(Pose.Rotation.Vector());

	double Yaw = MaxYaw > 0.0f ? FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X)) : 0.0;
	double Pitch = MaxPitch > 0.0f ? FMath::RadiansToDegrees(FMath::Atan2(Local.Z, FVector2D(Local.X, Local.Y).Size())) : 0.0;

	// Outside the ellipse, both angles scale back onto it.
	const double Ellipse = (MaxYaw > 0.0f ? FMath::Square(Yaw / MaxYaw) : 0.0) + (MaxPitch > 0.0f ? FMath::Square(Pitch / MaxPitch) : 0.0);
	if (Ellipse > 1.0)
	{
		const double Scale = 1.0 / FMath::Sqrt(Ellipse);
		Yaw *= Scale;
		Pitch *= Scale;
	}

	FRotator Limited = (BaseRotation * FRotator(Pitch, Yaw, 0.0).Quaternion()).Rotator();
	Limited.Roll = Pose.Rotation.Roll;

	FKzCameraPose Result = Pose;
	Result.Rotation = Limited;
	return Result;
}

void UKzCameraModule_RotationLimit::DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const
{
	constexpr double Length = 300.0;
	constexpr int32 Segments = 32;

	const UWorld* World = GetWorld();
	const FQuat BaseRotation = GetBase(Context);

	// The cone's rim, with four ribs from the camera.
	FVector Previous = FVector::ZeroVector;
	for (int32 Index = 0; Index <= Segments; ++Index)
	{
		const double Angle = 2.0 * UE_DOUBLE_PI * Index / Segments;
		const FRotator Edge(MaxPitch * FMath::Sin(Angle), MaxYaw * FMath::Cos(Angle), 0.0);
		const FVector Point = Pose.Location + BaseRotation.RotateVector(Edge.Vector()) * Length;

		if (Index > 0)
		{
			DrawDebugLine(World, Previous, Point, FColor::Magenta);
		}
		if (Index % (Segments / 4) == 0)
		{
			DrawDebugLine(World, Pose.Location, Point, FColor::Magenta);
		}
		Previous = Point;
	}

	DrawDebugLine(World, Pose.Location, Pose.Location + Pose.Rotation.Vector() * Length, FColor::White);
}

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
		Result.FocusDistance = FVector::Dist(Pose.Location, Context.TargetLocation);
	}
	else
	{
		return Result;
	}

	Result.Aperture = Aperture;
	return Result;
}
