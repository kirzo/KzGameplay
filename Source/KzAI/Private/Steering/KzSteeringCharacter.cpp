// Copyright 2026 kirzo

#include "Steering/KzSteeringCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

AKzSteeringCharacter::AKzSteeringCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		return MoveComp->MaxWalkSpeed;
	}
	return 0.0f;
}

float AKzSteeringCharacter::GetAgentMaxAcceleration() const
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		return MoveComp->GetMaxAcceleration();
	}
	return 0.0f;
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