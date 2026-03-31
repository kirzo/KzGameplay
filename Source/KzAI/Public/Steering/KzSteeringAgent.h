// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KzSteeringAgent.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UKzSteeringAgent : public UInterface
{
	GENERATED_BODY()
};

/** Interface for actors that can be steered by the KzSteeringComponent. */
class KZAI_API IKzSteeringAgent
{
	GENERATED_BODY()

public:
	/** Returns the agent's current location in world space. */
	virtual FVector GetAgentLocation() const = 0;

	/** Returns the agent's current velocity. */
	virtual FVector GetAgentVelocity() const = 0;

	/** Returns the agent's maximum speed. */
	virtual float GetAgentMaxSpeed() const = 0;

	/** Returns the agent's maximum acceleration (used to normalize steering forces into input vectors). */
	virtual float GetAgentMaxAcceleration() const = 0;

	/** Optional: Returns the agent's forward direction. */
	virtual FVector GetAgentDirection() const { return FVector::ForwardVector; }

	/** 
	 * Applies the calculated steering input to the agent.
	 * @param InputVector The normalized or clamped input vector [-1.0, 1.0].
	 */
	virtual void ApplySteeringInput(const FVector& InputVector) = 0;
};