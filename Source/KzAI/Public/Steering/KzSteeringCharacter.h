// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Steering/KzSteeringAgent.h"
#include "KzSteeringCharacter.generated.h"

/**
 * A lightweight base Character that natively implements the Steering Agent interface.
 * Routes steering requests directly to the standard Character Movement Component for maximum performance.
 */
UCLASS(Abstract, Blueprintable)
class KZAI_API AKzSteeringCharacter : public ACharacter, public IKzSteeringAgent
{
	GENERATED_BODY()

public:
	AKzSteeringCharacter(const FObjectInitializer& ObjectInitializer);

	//~ Begin IKzSteeringAgent Interface
	virtual float GetAgentRadius() const override;
	virtual FVector GetAgentLocation() const override;
	virtual FVector GetAgentVelocity() const override;
	virtual float GetAgentMaxSpeed() const override;
	virtual float GetAgentMaxAcceleration() const override;
	virtual FVector GetAgentDirection() const override;
	virtual void ApplySteeringInput(const FVector& InputVector) override;
	//~ End IKzSteeringAgent Interface
};