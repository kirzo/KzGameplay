// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzCameraTypes.generated.h"

class AActor;
class UKzCameraModule;
class UKzCameraRigComponent;
class UKzSplineFollowerComponent;

/** When a camera module runs. Modules run by stage, so their order doesn't depend on which layer brings them. */
UENUM(BlueprintType)
enum class EKzCameraStage : uint8
{
	/** Moves the distance along the follower's spline; the rig then hangs the camera from that point. */
	Rail,
	Position,

	/** Points the camera, replacing its rotation. */
	Aim,

	/** Turns the camera from where Aim left it. */
	Rotation,
	Lens
};

UENUM(BlueprintType)
enum class EKzCameraSpace : uint8
{
	/** The frame of the point the camera hangs from: the rail point, or the rig's parent without a follower. */
	Reference,
	Camera,
	World
};

/** What the camera looks like this frame. Modules take it and return it changed. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzCameraPose
{
	GENERATED_BODY()

	/** Distance along the follower's spline the camera hangs from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (Units = "Centimeters"))
	float RailDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (Units = "Degrees"))
	float FieldOfView = 90.0f;

	/** 0 leaves the camera's focus alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (Units = "Centimeters"))
	float FocusDistance = 0.0f;

	/** F-stop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float Aperture = 22.0f;

	/** A side without focus takes the other's distance with a closed aperture, so the blur fades in and out. */
	static FKzCameraPose Blend(const FKzCameraPose& A, const FKzCameraPose& B, float Alpha);
};

/** What a camera module knows besides the pose. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzCameraContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UKzCameraRigComponent> Rig = nullptr;

	/** The follower moving the camera's actor, if any. */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UKzSplineFollowerComponent> Follower = nullptr;

	/** What the camera follows. */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	TObjectPtr<AActor> Target = nullptr;

	/** Frame of the point the camera hangs from. Before the rail stage, the rig's parent; after it, the rail point. */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	FTransform Reference;

	/** Spline point closest to the target, before rail modules such as the lag move away from it. */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	FVector ProjectedLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	float DeltaTime = 0.0f;

	/** The camera jumped this frame (start, respawn); smoothing modules snap. */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bTeleport = false;
};

/** A layer of camera modules brought by a volume, a marker or Blueprint. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzCameraLayerSettings
{
	GENERATED_BODY()

	/** Each module replaces the one of its kind from the layers below. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Camera")
	TArray<TObjectPtr<UKzCameraModule>> Modules;

	/** Higher wins. Among equals, the latest activated wins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (InlineEditConditionToggle))
	bool bOverrideBlendTime = false;

	/** Time to blend in and out. Without it, the rig's default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (EditCondition = "bOverrideBlendTime", ClampMin = "0", Units = "Seconds"))
	float BlendTime = 1.0f;

	/**
	 * While active, hides every layer activated before it, whatever their priority: the camera starts again from the rig's modules plus this layer's.
	 * With no modules it returns the camera to its defaults. When it goes away, the hidden layers come back.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bClearPrevious = false;
};
