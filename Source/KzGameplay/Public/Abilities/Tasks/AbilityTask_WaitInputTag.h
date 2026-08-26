// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AbilityTask_WaitInputTag.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzInputTagTaskDelegate, FGameplayTag, InputTag);

/**
 * Waits on the ability's own input bindings and reports which one arrived.
 *
 * Replaces WaitInputPress/WaitInputRelease, which ride the engine's generic InputPressed event: that event
 * carries no payload, so an ability bound to several inputs cannot tell them apart. Here the tag comes out
 * as a pin, ready to switch on.
 */
UCLASS()
class KZGAMEPLAY_API UAbilityTask_WaitInputTag : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_WaitInputTag(const FObjectInitializer& ObjectInitializer);

	/** Fired when one of the watched inputs is pressed. */
	UPROPERTY(BlueprintAssignable)
	FKzInputTagTaskDelegate OnPressed;

	/** Fired when one of the watched inputs is released. */
	UPROPERTY(BlueprintAssignable)
	FKzInputTagTaskDelegate OnReleased;

	/**
	 * Waits for the ability's input bindings, reporting which one fired.
	 * @param InputTags Which inputs to watch. Leave empty to watch every input the ability binds.
	 * @param bTriggerOnce Ends the task after the first press or release instead of staying alive.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitInputTag* WaitInputTag(UGameplayAbility* OwningAbility, FGameplayTagContainer InputTags, bool bTriggerOnce = false);

	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

private:
	/** Inputs we care about. Empty means whatever the ability binds. */
	UPROPERTY()
	FGameplayTagContainer WatchedTags;

	UPROPERTY()
	bool bEndOnFirst = false;

	UFUNCTION()
	void HandleInputTag(FGameplayTag InputTag, bool bPressed);

	class UKzAbilitySystemComponent* GetKzAbilitySystem() const;
};