// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KzGameplayAbility.generated.h"

/** Defines how an ability reacts to one input tag. */
UENUM(BlueprintType)
enum class EKzAbilityInputPolicy : uint8
{
	/** The ability does not care about input directly. */
	None,

	/** The ability listens to the input for WaitInput nodes, but will NOT activate from it. */
	ListenOnly,

	/** The ability can be activated by the input, and will also listen to it while active. */
	ActivateAndListen,

	/**
	 * The input is turned into a gameplay event carrying its own tag, for abilities that would rather
	 * activate through their AbilityTriggers, with the payload and conditions that brings.
	 */
	TriggerEvent
};

/** Binds one input tag to how this ability reacts to it. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzAbilityInput
{
	GENERATED_BODY()

	/** The input this entry is about. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;

	/** What the ability does when that input arrives. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	EKzAbilityInputPolicy Policy = EKzAbilityInputPolicy::None;
};

/**
 * Base Gameplay Ability class for the KzGameplay plugin.
 * Provides built-in support for input mapping, auto-activation, and custom cancel triggers.
 */
UCLASS(Abstract)
class KZGAMEPLAY_API UKzGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility();

	/**
	 * Inputs this ability reacts to, and how. An ability may bind several: one to activate it, others to
	 * drive it while it runs, which is what lets a single ability offer more than one action.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (TitleProperty = "InputTag"))
	TArray<FKzAbilityInput> InputBindings;

	/** Tells an ability to activate immediately when it's granted. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool bActivateAbilityOnGranted;

	/** Triggers to determine if this ability should cancel in response to an event */
	UPROPERTY(EditDefaultsOnly, Category = "Triggers")
	TArray<FAbilityTriggerData> AbilityCancelTriggers;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void PostLoad() override;

	/** How this ability reacts to an input, or None if it does not bind it. */
	UFUNCTION(BlueprintPure, Category = "Ability|Input")
	EKzAbilityInputPolicy GetInputPolicy(FGameplayTag QueryTag) const;

	/** Every input this ability binds, whatever the policy. */
	UFUNCTION(BlueprintPure, Category = "Ability|Input")
	FGameplayTagContainer GetBoundInputTags() const;

	/**
	 * Whether an input is being held right now, for abilities that poll instead of waiting on a task.
	 * Takes a tag because an ability may bind several, and reports the input itself: it answers the same
	 * whether or not this ability binds it.
	 */
	UFUNCTION(BlueprintPure, Category = "Ability|Input")
	bool IsInputPressed(FGameplayTag QueryTag) const;

private:
	/**
	 * Single binding from before InputBindings existed. Names kept so old assets still load, and migrated
	 * into the array on PostLoad. Hidden rather than removed for that one reason.
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use InputBindings instead."))
	EKzAbilityInputPolicy InputPolicy = EKzAbilityInputPolicy::None;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use InputBindings instead."))
	FGameplayTag InputTag;
};