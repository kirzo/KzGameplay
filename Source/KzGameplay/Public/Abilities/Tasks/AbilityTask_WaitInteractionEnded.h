// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Interaction/KzInteractionTypes.h"
#include "AbilityTask_WaitInteractionEnded.generated.h"

class UKzInteractorComponent;
class UKzInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzInteractionEndedTaskDelegate, UKzInteractableComponent*, Interactable, EKzInteractionEndReason, Reason);

/**
 * Waits for the avatar's interaction to end, whoever ends it: the player letting go, the object breaking
 * the hold, the world taking it away. An ability driving a continuous interaction should wait on this
 * rather than on an event tag, since nothing guarantees that whoever ended it also sent an event.
 */
UCLASS()
class KZGAMEPLAY_API UAbilityTask_WaitInteractionEnded : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_WaitInteractionEnded(const FObjectInitializer& ObjectInitializer);

	/** Fired once the interaction ends, with what ended it. */
	UPROPERTY(BlueprintAssignable)
	FKzInteractionEndedTaskDelegate OnInteractionEnded;

	/**
	 * Waits until the interaction the avatar is engaged in ends.
	 * Fires immediately with Interrupted if there is nothing to wait for, so an ability can never hang here.
	 * @param Interactor Whose interaction to watch. Leave empty to use the avatar's own interactor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitInteractionEnded* WaitInteractionEnded(UGameplayAbility* OwningAbility, UKzInteractorComponent* Interactor = nullptr);

	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

private:
	/** The interactor we are listening to. */
	UPROPERTY()
	TObjectPtr<UKzInteractorComponent> Interactor;

	UFUNCTION()
	void HandleInteractionEnded(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason);
};
