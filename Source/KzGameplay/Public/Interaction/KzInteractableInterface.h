// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KzInteractableInterface.generated.h"

class UKzInteractorComponent;
class UKzInteractableComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class UKzInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/** Interface for actors and components that can be interacted with. */
class KZGAMEPLAY_API IKzInteractableInterface
{
	GENERATED_BODY()

public:
	/**
	 * Called when an interactor successfully executes an interaction on this target.
	 * @param Interactor The component that initiated the interaction.
	 * @return True if the interaction was handled and consumed successfully.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool HandleInteraction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);
};