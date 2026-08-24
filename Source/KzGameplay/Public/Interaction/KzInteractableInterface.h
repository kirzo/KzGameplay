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

/**
 * Interface for actors and components that can be interacted with.
 *
 * Networking: the server decides, every machine mirrors, so HandleInteraction and OnInteractionEnded run on
 * server, owning client and simulated proxies alike. Guard authoritative work (spawning, scoring, damage)
 * with HasAuthority and leave local work (input, effects, audio) unguarded.
 * ShouldKeepInteractionAlive is only asked on the authority.
 */
class KZGAMEPLAY_API IKzInteractableInterface
{
	GENERATED_BODY()

public:
	/** Called to determine if this interaction is currently allowed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);

	/**
	 * Called when an interactor successfully executes an interaction on this target.
	 * Register undo work on the Interaction: it runs when the interaction ends, however it ends.
	 * Keep its Handle to end the interaction yourself later.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EKzInteractionResult HandleInteraction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction);

	/**
	 * Called once an interaction has ended, whoever ended it, with its components possibly already gone.
	 * Registered cleanup has run by now, so this is for reacting to the outcome, not for undoing things.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason);

	/**
	 * Called periodically while the interaction runs. Return false to end it, filling in why.
	 * Plain range is already covered by KeepAliveRange, so this is for rules only you know.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool ShouldKeepInteractionAlive(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason);

protected:
	virtual bool CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) { return true; }
	virtual void OnInteractionEnded_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason Reason) {}
	virtual bool ShouldKeepInteractionAlive_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason) { return true; }
};
