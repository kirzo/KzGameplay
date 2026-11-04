// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Camera/KzCameraModule.h"
#include "Components/KzComponentReference.h"
#include "Misc/KzTransformSource.h"
#include "KzCameraModules.generated.h"

UENUM(BlueprintType)
enum class EKzCameraLookAt : uint8
{
	/** A fixed world direction. */
	Direction,

	/** The rig's target. */
	Target,

	/** A component, socket or world point. */
	Point
};

UENUM(BlueprintType)
enum class EKzCameraFocus : uint8
{
	Distance,

	/** The rig's target. */
	Target
};

/** Makes the camera trail behind the target along the spline. */
UCLASS(meta = (DisplayName = "Lag"))
class KZGAMEPLAY_API UKzCameraModule_Lag : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_Lag();

	/** How far behind, in time, the camera trails a target moving at constant speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lag", meta = (ClampMin = "0", Units = "Seconds"))
	float LagTime = 0.3f;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	float Distance = 0.0f;
	float Rate = 0.0f;
	bool bInitialized = false;
};

/** Places the camera ahead of or behind the target's point on the spline. */
UCLASS(meta = (DisplayName = "Spline Offset"))
class KZGAMEPLAY_API UKzCameraModule_SplineOffset : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_SplineOffset();

	/** Positive is ahead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Offset", meta = (Units = "Centimeters"))
	float Offset = 0.0f;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;
};

UCLASS(meta = (DisplayName = "Position Offset"))
class KZGAMEPLAY_API UKzCameraModule_PositionOffset : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position Offset")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position Offset")
	EKzCameraSpace Space = EKzCameraSpace::Reference;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;
};

/** Moves the camera off the spline, in a plane, to keep the target where its spline point would be on screen. */
UCLASS(meta = (DisplayName = "Pan"))
class KZGAMEPLAY_API UKzCameraModule_Pan : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_Pan();

	/** Normal of the plane the camera pans in, in the reference frame. Zero pans in the view plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pan")
	FVector PlaneNormal = FVector::ZeroVector;

	/** Farthest the camera pans; it slows down as it gets close. 0 means no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pan", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pan", meta = (ClampMin = "0", Units = "Seconds"))
	float SmoothingTime = 0.3f;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	FVector Offset = FVector::ZeroVector;
	FVector Rate = FVector::ZeroVector;
	bool bInitialized = false;
};

UCLASS(meta = (DisplayName = "Look At"))
class KZGAMEPLAY_API UKzCameraModule_LookAt : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_LookAt();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At")
	EKzCameraLookAt Mode = EKzCameraLookAt::Target;

	/** World direction to look along. The default suits a level that runs along X, seen from its side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At", meta = (EditCondition = "Mode == EKzCameraLookAt::Direction", EditConditionHides))
	FVector Direction = FVector(0.0f, -1.0f, 0.0f);

	/** Added to the target's location, e.g. to aim at the chest instead of the feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At", meta = (EditCondition = "Mode == EKzCameraLookAt::Target", EditConditionHides))
	FVector TargetOffset = FVector::ZeroVector;

	/** A component or socket plus its offset. With nothing picked, the offset is a world location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At", meta = (EditCondition = "Mode == EKzCameraLookAt::Point", EditConditionHides))
	FKzComponentSocketReference Point;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	/** Point, resolved once. */
	FKzTransformSource PointSource;
};

/** Turns the camera from where it is aimed, in its own frame. */
UCLASS(meta = (DisplayName = "Rotation Offset"))
class KZGAMEPLAY_API UKzCameraModule_RotationOffset : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_RotationOffset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Offset")
	FRotator Offset = FRotator::ZeroRotator;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;
};

UCLASS(meta = (DisplayName = "Field Of View"))
class KZGAMEPLAY_API UKzCameraModule_FieldOfView : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_FieldOfView();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Of View", meta = (UIMin = "5", UIMax = "170", Units = "Degrees"))
	float FieldOfView = 90.0f;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;
};

/** Focus for a cine camera. Blending it in and out opens and closes the aperture. */
UCLASS(meta = (DisplayName = "Depth Of Field"))
class KZGAMEPLAY_API UKzCameraModule_DepthOfField : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_DepthOfField();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Depth Of Field")
	EKzCameraFocus Focus = EKzCameraFocus::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Depth Of Field", meta = (ClampMin = "1", Units = "Centimeters", EditCondition = "Focus == EKzCameraFocus::Distance", EditConditionHides))
	float FocusDistance = 1000.0f;

	/** F-stop. Lower blurs more. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Depth Of Field", meta = (ClampMin = "1"))
	float Aperture = 2.8f;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;
};
