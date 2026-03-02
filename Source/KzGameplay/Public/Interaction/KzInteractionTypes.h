// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzInteractionTypes.generated.h"

/** Defines the outcome of an interaction attempt. */
UENUM(BlueprintType)
enum class EKzInteractionResult : uint8
{
	/** The interaction failed or was ignored. The interactor should cancel the action. */
	Ignored,

	/** The interaction was an instant success (e.g., picking up an item). The interactor can finish the action. */
	Completed,

	/** The interaction is ongoing (e.g., holding a lever). The interactor must wait until StopInteraction is called. */
	Continuous
};