// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "KzAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzAbilityInputTagSignature, FGameplayTag, InputTag, bool, bPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzGameplayEventSignature, const FGameplayTag&, Tag, FGameplayEventData, EventData);

/**
 * Custom Ability System Component for KzGameplay.
 * Handles Input Tag routing, custom ability cancellation triggers, and global gameplay events.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UKzAbilitySystemComponent();

	/** Fired whenever a gameplay event is handled by this ASC. Useful for UI or generic listeners. */
	UPROPERTY(BlueprintAssignable, Category = "GAS|Events")
	FKzGameplayEventSignature OnGameplayEvent;

	/**
	 * Fired for every bound input, carrying which one it was. Ability tasks listen here instead of the
	 * engine's generic InputPressed event, which has no room for a tag and so cannot tell them apart.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Abilities|Input")
	FKzAbilityInputTagSignature OnAbilityInputTag;

	/** Whether an input is being held right now. Answers per tag, which a spec's single flag cannot. */
	UFUNCTION(BlueprintPure, Category = "Abilities|Input")
	bool IsInputTagPressed(FGameplayTag InputTag) const { return PressedInputTags.Contains(InputTag); }

	/** Routes a pressed input tag to the relevant abilities. */
	UFUNCTION(BlueprintCallable, Category = "GAS|Input")
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/** Routes a released input tag to the relevant abilities. */
	UFUNCTION(BlueprintCallable, Category = "GAS|Input")
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

protected:
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;

	/** Inputs currently held, tracked here because it is where both press and release arrive. */
	TSet<FGameplayTag> PressedInputTags;

	/** Abilities that should be cancelled when a specific gameplay event is fired. */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle>> GameplayEventCancelledAbilities;

	/** Abilities that should be cancelled when a specific tag is added to this ASC. */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle>> OwnedTagCancelledAbilities;

	/** Active delegate handles for our tag cancellation listeners. */
	TMap<FGameplayTag, FDelegateHandle> CancelTagDelegateHandles;

	/** Callback executed when a tag that might cancel an ability changes. */
	virtual void CancelTagChanged(const FGameplayTag Tag, int32 NewCount);
};