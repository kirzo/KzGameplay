// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Camera/KzCameraModule.h"
#include "Components/KzComponentReference.h"
#include "Misc/KzTransformSource.h"
#include "KzCameraModules.generated.h"

UENUM(BlueprintType)
enum class EKzCameraDeadZoneUnits : uint8
{
	Centimeters,

	/** Fractions of half the screen, at the focus's depth. Depth stays in centimetres. */
	Screen
};

UENUM(BlueprintType)
enum class EKzCameraLeadSource : uint8
{
	/** Where the pawn's player wants to go, at its top speed. Moves without input (attacks, knockbacks) don't count. */
	MovementInput,

	/** The target's velocity, whatever moves it. */
	Velocity
};

UENUM(BlueprintType)
enum class EKzCameraLookAt : uint8
{
	/** A fixed world direction. */
	Direction,

	/** The rig's target. */
	Target,

	/** The focus: the target as the focus stages leave it, smoothed and led. */
	Focus,

	/** A component, socket or world point. */
	Point
};

UENUM(BlueprintType)
enum class EKzCameraRotationBase : uint8
{
	/** Where the camera points hanging from its anchor, before aiming. */
	Rest,

	/** A fixed world direction. */
	World
};

UENUM(BlueprintType)
enum class EKzCameraFocus : uint8
{
	Distance,

	/** The rig's target. */
	Target
};

/** Lets the target move inside a box around the focus without moving it; leaving the box drags the focus along. Axes follow the view. */
UCLASS(meta = (DisplayName = "Dead Zone"))
class KZGAMEPLAY_API UKzCameraModule_DeadZone : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_DeadZone();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Zone")
	EKzCameraDeadZoneUnits Units = EKzCameraDeadZoneUnits::Centimeters;

	/** Half width, across the screen. 0 means no dead zone on that axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Zone", meta = (ClampMin = "0"))
	float Horizontal = 100.0f;

	/** Half height, up and down the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Zone", meta = (ClampMin = "0"))
	float Vertical = 50.0f;

	/** Half depth, towards and away from the camera. Always in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Zone", meta = (ClampMin = "0", Units = "Centimeters"))
	float Depth = 0.0f;

	virtual void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const override;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	FVector Center = FVector::ZeroVector;
	FVector HalfSize = FVector::ZeroVector;
	bool bInitialized = false;

	/** Depth, horizontal and vertical half sizes in centimetres, along the view's X, Y and Z. */
	FVector GetHalfSize(const FKzCameraContext& Context) const;
};

/** Moves the focus ahead of the target as it moves, to show more of where it is going. Only across the screen, never in depth. */
UCLASS(meta = (DisplayName = "Look Ahead"))
class KZGAMEPLAY_API UKzCameraModule_LookAhead : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_LookAhead();

	/** Velocity is used when the target isn't a pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look Ahead")
	EKzCameraLeadSource Source = EKzCameraLeadSource::MovementInput;

	/** How far ahead, in time, at the source's speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look Ahead", meta = (ClampMin = "0", Units = "Seconds"))
	float LeadTime = 0.5f;

	/** 0 turns the lead off, e.g. in a layer that overrides it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look Ahead", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxLead = 200.0f;

	/** How long the lead takes to settle when the target turns or stops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look Ahead", meta = (ClampMin = "0", Units = "Seconds"))
	float SmoothingTime = 0.5f;

	/**
	 * Keeps the lead while the source is still, instead of easing back to the target: letting go of the input keeps showing where it was heading.
	 * With Velocity, what is kept is what is left once the target has braked.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look Ahead")
	bool bHoldWhenIdle = true;

	/** Time standing still before the lead starts easing back, when it isn't held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look Ahead", meta = (ClampMin = "0", Units = "Seconds", EditCondition = "!bHoldWhenIdle"))
	float ReturnDelay = 0.5f;

	virtual void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const override;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	float IdleTime = 0.0f;
	FVector DesiredLead = FVector::ZeroVector;
	FVector Lead = FVector::ZeroVector;
	FVector Rate = FVector::ZeroVector;
	bool bInitialized = false;
};

/** Makes the focus trail the target, with optional caps so sudden moves don't drag the camera along at their speed. */
UCLASS(meta = (DisplayName = "Lag"))
class KZGAMEPLAY_API UKzCameraModule_Lag : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_Lag();

	/** How far behind, in time, the focus trails a target moving at constant speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lag", meta = (ClampMin = "0", Units = "Seconds"))
	float LagTime = 0.3f;

	/** Top speed of the focus. Keep it above the target's normal top speed, so it only bites on sudden moves. 0 means no cap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lag", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float MaxSpeed = 0.0f;

	/** Top acceleration of the focus, which also brakes it in time to stop on the target. 0 means no cap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lag", meta = (ClampMin = "0", Units = "CentimetersPerSecondSquared"))
	float MaxAcceleration = 0.0f;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	bool bInitialized = false;
};

/** Hangs the camera ahead of or behind its point on the follower's spline. Does nothing without a spline. */
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

/** Hangs the camera from the focus instead of its anchor, for cameras without a spline. Locked world axes keep the anchor's value. */
UCLASS(meta = (DisplayName = "Follow"))
class KZGAMEPLAY_API UKzCameraModule_Follow : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_Follow();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	bool bLockX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	bool bLockY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	bool bLockZ = false;

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

/**
 * Moves the camera, in a plane, towards the focus it hangs away from: off the spline with a follower, or off a fixed anchor.
 * Pointless with Follow, which already hangs the camera from the focus.
 */
UCLASS(meta = (DisplayName = "Pan"))
class KZGAMEPLAY_API UKzCameraModule_Pan : public UKzCameraModule
{
	GENERATED_BODY()

public:
	/** Normal of the plane the camera pans in, in the reference frame. Zero pans in the view plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pan")
	FVector PlaneNormal = FVector::ZeroVector;

	/** Farthest the camera pans; it slows down as it gets close. 0 means no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pan", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxDistance = 300.0f;

	virtual void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const override;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	FVector GetPlaneNormal(const FKzCameraContext& Context, const FKzCameraPose& Pose) const;
};

UCLASS(meta = (DisplayName = "Look At"))
class KZGAMEPLAY_API UKzCameraModule_LookAt : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_LookAt();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At")
	EKzCameraLookAt Mode = EKzCameraLookAt::Focus;

	/** World direction to look along. The default suits a level that runs along X, seen from its side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At", meta = (EditCondition = "Mode == EKzCameraLookAt::Direction", EditConditionHides))
	FVector Direction = FVector(0.0f, -1.0f, 0.0f);

	/** Added to the target's or the focus's location, e.g. to aim at the chest instead of the feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look At", meta = (EditCondition = "Mode == EKzCameraLookAt::Target || Mode == EKzCameraLookAt::Focus", EditConditionHides))
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

/** Keeps the camera pointing within an elliptical cone, so aiming can't undo the framing. A perfect cone when both angles match. */
UCLASS(meta = (DisplayName = "Rotation Limit"))
class KZGAMEPLAY_API UKzCameraModule_RotationLimit : public UKzCameraModule
{
	GENERATED_BODY()

public:
	UKzCameraModule_RotationLimit();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Limit")
	EKzCameraRotationBase Base = EKzCameraRotationBase::Rest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Limit", meta = (EditCondition = "Base == EKzCameraRotationBase::World", EditConditionHides))
	FVector Direction = FVector(0.0f, -1.0f, 0.0f);

	/** Furthest turn left or right from the base. 0 locks it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Limit", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MaxYaw = 15.0f;

	/** Furthest turn up or down from the base. 0 locks it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Limit", meta = (ClampMin = "0", ClampMax = "89", Units = "Degrees"))
	float MaxPitch = 10.0f;

	virtual void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const override;

protected:
	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) override;

private:
	FQuat GetBase(const FKzCameraContext& Context) const;
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
