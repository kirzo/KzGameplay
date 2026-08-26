// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "Interaction/KzInteractionTypes.h"
#include "KzGameplayAbility_InteractionAction.generated.h"

class UAnimMontage;
class UKzInteractorComponent;

/**
 * Drives the actions an interactable offers while an interaction runs: press, play, land the effect,
 * repeat. One ability covers every object that works that way, since the montage, the notify and the
 * effect are data on the interactable rather than a graph per object.
 *
 * Bind it to the inputs it should watch through InputBindings, and give ResolveMontage a body so the
 * avatar answers an animation tag with a montage from its own set.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_InteractionAction : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_InteractionAction();

	/**
	 * Turns an action's animation tag into a montage for this avatar.
	 * Left empty on purpose: which montage answers a tag is a question about the character, not the object.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction Action")
	UAnimMontage* ResolveMontage(FGameplayTag AnimationTag) const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** The action being played right now, if any. Further presses are ignored until it lands. */
	UPROPERTY()
	FGameplayTag RunningAction;

	UFUNCTION()
	void OnInputPressed(FGameplayTag PressedTag);

	UFUNCTION()
	void OnInteractionEnded(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason);

	UFUNCTION()
	void OnEffectNotify(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	/** Runs the pending action on the interactable. Clearing it belongs to whoever knows it is over. */
	void CommitRunningAction();

	UKzInteractorComponent* GetInteractor() const;
};