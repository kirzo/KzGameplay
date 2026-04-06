// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KzSteeringBehavior.generated.h"

class IKzSteeringAgent;
class UKzSteeringComponent;

/** Abstract base class for instanced steering behaviors. */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class KZAI_API UKzSteeringBehavior : public UObject
{
	GENERATED_BODY()

public:
	/** How much this behavior influences the final steering force. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering", meta = (ExposeOnSpawn, ClampMin = "0.0"))
	float Weight = 1.0f;

	/** Should calculations be strictly 2D (ignore Z axis)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering", meta = (ExposeOnSpawn))
	bool bForce2D = true;

	/** Called once when the component activates or the behavior is added. */
	virtual void InitBehavior(UKzSteeringComponent* OwnerComponent, IKzSteeringAgent* Agent) {}

	/** Computes the unweighted steering force for this frame. */
	virtual FVector ComputeForce(const UKzSteeringComponent* OwnerComponent, const IKzSteeringAgent* Agent, float DeltaTime) PURE_VIRTUAL(UKzSteeringBehavior::ComputeForce, return FVector::ZeroVector;);
};