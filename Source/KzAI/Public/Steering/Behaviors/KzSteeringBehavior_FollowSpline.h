// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Steering/KzSteeringBehavior.h"
#include "Components/KzComponentReference.h"
#include "KzSteeringBehavior_FollowSpline.generated.h"

UENUM(BlueprintType)
enum class EKzSplineEndBehavior : uint8
{
	Stop        UMETA(DisplayName = "Stop at End"),
	Reverse     UMETA(DisplayName = "Reverse Direction"),
	Continue    UMETA(DisplayName = "Continue Straight")
};

UCLASS(DisplayName = "Follow Spline")
class KZAI_API UKzSteeringBehavior_FollowSpline : public UKzSteeringBehavior
{
	GENERATED_BODY()

public:
	UKzSteeringBehavior_FollowSpline();

	/** Reference to the Spline Component to follow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline", meta = (AllowedClasses = "/Script/Engine.SplineComponent"))
	FKzComponentReference SplineReference;

	/** Should the agent follow the spline from start to end (Forward) or end to start (Backward)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline")
	bool bFollowForward = true;

	/** What to do when the agent reaches the end of the spline (only applies to non-closed splines). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline")
	EKzSplineEndBehavior EndBehavior = EKzSplineEndBehavior::Stop;

	/** How far ahead on the spline the 'carrot' target should be placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline", meta = (ClampMin = "10.0"))
	float CarrotDistance = 300.0f;

	/** How far into the future (in seconds) we predict the agent's position to find the closest point on the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline", meta = (ClampMin = "0.0"))
	float PredictionTime = 0.5f;

	/** Radius used to slow down when stopping at the end of the spline (if EndBehavior == Stop). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline", meta = (ClampMin = "10.0"))
	float SlowingRadius = 250.0f;

	/** If true, draws the predicted position, the closest point on the spline, and the carrot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Spline|Debug")
	bool bShowDebug = false;

	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) override;
};