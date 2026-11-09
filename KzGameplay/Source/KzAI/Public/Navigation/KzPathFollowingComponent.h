// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "KzPathFollowingComponent.generated.h"

class UKzInputHandlerComponent;

/**
 * Follows paths through the pawn's Move input, as a player would with a stick, so whatever ignores or reshapes a player's
 * movement does the same to an AI: Input Block Windows, walk speeds, acceleration. A pawn without an input handler moves
 * the engine's way.
 */
UCLASS(ClassGroup = (KzAI))
class KZAI_API UKzPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

public:
	virtual void OnPathFinished(const FPathFollowingResult& Result) override;
	virtual void PauseMove(FAIRequestID RequestID = FAIRequestID::CurrentRequest, EPathFollowingVelocityMode VelocityMode = EPathFollowingVelocityMode::Reset) override;

protected:
	virtual void FollowPathSegment(float DeltaTime) override;

private:
	UKzInputHandlerComponent* GetInputHandler() const;

	/** Lets go of the stick once the path stops. */
	void ReleaseMove();
};
