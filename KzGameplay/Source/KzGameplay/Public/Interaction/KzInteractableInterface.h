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
 * Networking: the server decides, every machine mirrors, so OnInteractionBegun and OnInteractionEnded run
 * on server, owning client and simulated proxies alike. Guard authoritative work (spawning, scoring,
 * damage) with HasAuthority and leave local work (input, effects, audio) unguarded.
 * GetInteractionResult and ShouldKeepInteractionAlive are only ever asked on the authority.
 */
class KZGAMEPLAY_API IKzInteractableInterface
{
	GENERATED_BODY()

public:
	/** Called to determine if this interaction is currently allowed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);

	/**
	 * Asked before anything runs: is this interaction instant, ongoing, or not happening at all?
	 * A pure question, with no side effects, because the answers of every handler are still being weighed.
	 * Return Ignored to abstain and let the interactable's DefaultInteractionResult stand.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EKzInteractionResult GetInteractionResult(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable);

	/**
	 * Called once the interaction exists and is confirmed. This is where the work goes.
	 * Register undo work on the Interaction: it runs when the interaction ends, however it ends.
	 * Keep its Handle to end the interaction yourself later.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteractionBegun(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction);

	/**
	 * Called once an interaction has ended, whoever ended it, with its components possibly already gone.
	 * Registered cleanup has run by now, so this is for reacting to the outcome, not for undoing things.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason);

	/**
	 * Called when one of the interactable's actions runs, after its data-driven Effect.
	 * This is where a component answers an action in C++, the way the pump adds pressure.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteractionAction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag);

	/**
	 * Called while this is focused, to report whether the interaction could run right now.
	 * Return false with a reason to stay visible but blocked, so the UI can explain itself.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool GetInteractionAvailability(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag& OutReason);

	/**
	 * Called periodically while the interaction runs. Return false to end it, filling in why.
	 * Plain range is already covered by KeepAliveRange, so this is for rules only you know.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool ShouldKeepInteractionAlive(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason);

protected:
	virtual bool CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) { return true; }
	virtual EKzInteractionResult GetInteractionResult_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) { return EKzInteractionResult::Ignored; }
	virtual void OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction) {}
	virtual void OnInteractionEnded_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason Reason) {}
	virtual void OnInteractionAction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag InputTag) {}
	virtual bool GetInteractionAvailability_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FGameplayTag& OutReason) { return true; }
	virtual bool ShouldKeepInteractionAlive_Implementation(const FKzInteraction& Interaction, EKzInteractionEndReason& OutReason) { return true; }
};
