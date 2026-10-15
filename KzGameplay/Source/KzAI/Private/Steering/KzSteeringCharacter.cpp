// Copyright 2026 kirzo

#include "Steering/KzSteeringCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

AKzSteeringCharacter::AKzSteeringCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float AKzSteeringCharacter::GetAgentRadius() const
{
	return GetSimpleCollisionRadius();
}

FVector AKzSteeringCharacter::GetAgentLocation() const
{
	return GetActorLocation();
}

FVector AKzSteeringCharacter::GetAgentVelocity() const
{
	return GetVelocity();
}

float AKzSteeringCharacter::GetAgentMaxSpeed() const
{
	return GetCharacterMovement() ? GetCharacterMovement()->GetMaxSpeed() : 0.0f;
}

float AKzSteeringCharacter::GetAgentMaxAcceleration() const
{
	return GetCharacterMovement() ? GetCharacterMovement()->GetMaxAcceleration() : 0.0f;
}

FVector AKzSteeringCharacter::GetAgentDirection() const
{
	return GetActorForwardVector();
}

void AKzSteeringCharacter::ApplySteeringInput(const FVector& InputVector)
{
	// Route the steering force directly to Unreal's native movement system
	AddMovementInput(InputVector);
}

void AKzSteeringCharacter::ApplySteeringForce(const FVector& VelocityToApply)
{
	GetCharacterMovement()->RequestDirectMove(VelocityToApply, false);
}