// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interaction/KzInteractionTypes.h"
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
	 * @param Interactable The specific interactable component that was targeted.
	 * @return The result of the interaction (Ignored, Completed, or Continuous).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EKzInteractionResult HandleInteraction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);

	/**
	 * Called to manually abort an ongoing continuous interaction.
	 * @param Interactor The component that is stopping the interaction.
	 * @param Interactable The specific interactable component that was targeted.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void StopInteraction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);
};