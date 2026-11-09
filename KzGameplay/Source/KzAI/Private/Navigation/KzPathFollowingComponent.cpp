// Copyright 2026 kirzo

#include "Navigation/KzPathFollowingComponent.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"
#include "AIController.h"

void UKzPathFollowingComponent::OnPathFinished(const FPathFollowingResult& Result)
{
	ReleaseMove();
	Super::OnPathFinished(Result);
}

void UKzPathFollowingComponent::PauseMove(FAIRequestID RequestID, EPathFollowingVelocityMode VelocityMode)
{
	Super::PauseMove(RequestID, VelocityMode);

	if (Status == EPathFollowingStatus::Paused)
	{
		ReleaseMove();
	}
}

void UKzPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	UKzInputHandlerComponent* InputHandler = GetInputHandler();
	if (!InputHandler || !Path.IsValid() || NavMovementInterface == nullptr)
	{
		Super::FollowPathSegment(DeltaTime);
		return;
	}

	// The engine's acceleration-based following, braking included, handed to the stick instead of the movement component
	const FVector CurrentLocation = NavMovementInterface->GetFeetLocation();
	CurrentMoveInput = (GetCurrentTargetLocation() - CurrentLocation).GetSafeNormal();

	bIsDecelerating = false;
	if (bStopMovementOnFinish && MoveSegmentStartIndex >= DecelerationSegmentIndex)
	{
		const FVector::FReal DistToEndSq = FVector::DistSquared(CurrentLocation, Path->GetEndLocation());
		if (DistToEndSq < FMath::Square(CachedBrakingDistance))
		{
			bIsDecelerating = true;
			CurrentMoveInput *= FMath::Clamp(FMath::Sqrt(DistToEndSq) / CachedBrakingDistance, 0.0, 1.0);
		}
	}

	PostProcessMove.ExecuteIfBound(this, CurrentMoveInput);
	InputHandler->SetInputAxis(KzTags::Input::Move, CurrentMoveInput);
}

UKzInputHandlerComponent* UKzPathFollowingComponent::GetInputHandler() const
{
	const AController* Controller = Cast<AController>(GetOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
}

void UKzPathFollowingComponent::ReleaseMove()
{
	if (UKzInputHandlerComponent* InputHandler = GetInputHandler())
	{
		InputHandler->SetInputAxis(KzTags::Input::Move, FVector::ZeroVector);
	}
}
