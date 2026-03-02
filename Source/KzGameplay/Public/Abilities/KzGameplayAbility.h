// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KzGameplayAbility.generated.h"

/** Defines how an ability interacts with the input system. */
UENUM(BlueprintType)
enum class EKzAbilityInputPolicy : uint8
{
	/** The ability does not care about input directly. */
	None,

	/** The ability listens to the input for WaitInput nodes, but will NOT activate from it. */
	ListenOnly,

	/** The ability can be activated by the input, and will also listen to it while active. */
	ActivateAndListen
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

	// ==========================================
	// INPUT
	// ==========================================

	/** How this ability handles the specified InputTag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	EKzAbilityInputPolicy InputPolicy;

	/** The input tag associated with this ability (for activation or listening). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input", EditCondition = "InputPolicy != EKzAbilityInputPolicy::None"))
	FGameplayTag InputTag;

	// ==========================================
	// INITIALIZATION & TRIGGERS
	// ==========================================

	/** Tells an ability to activate immediately when it's granted. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool bActivateAbilityOnGranted;

	/** Triggers to determine if this ability should cancel in response to an event */
	UPROPERTY(EditDefaultsOnly, Category = "Triggers")
	TArray<FAbilityTriggerData> AbilityCancelTriggers;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};