// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "InputTriggers.h"
#include "KzInputTrigger_YieldToInteraction.generated.h"

/**
 * Triggers while the key is held, like a mapping without triggers, except for presses that begin while the player's interactor has
 * something focused or is in a continuous interaction: those belong to interaction until released. Put it on the mapping that shares
 * its key with interaction, such as the dodge button, so the key interacts when there is something to use and dodges otherwise.
 * The interaction action on that key must not consume input, nor this one, or Enhanced Input gives the key to only one of them.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Yield To Interaction"))
class KZGAMEPLAY_API UKzInputTrigger_YieldToInteraction : public UInputTrigger
{
	GENERATED_BODY()

protected:
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime) override;

private:
	/** Decided as the press begins, so picking the focused thing up while still holding the key doesn't fire the rest of the press. */
	bool bPressYielded = false;
};
