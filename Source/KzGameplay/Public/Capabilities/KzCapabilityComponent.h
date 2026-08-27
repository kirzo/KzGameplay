// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "KzCapabilityComponent.generated.h"

class UAbilitySystemComponent;
class UKzCapabilitySet;

/**
 * Grants and revokes capabilities, and is the only thing that should ever grant an ability.
 *
 * A capability is a gameplay tag on the avatar answering "can this actor do X", which is what the world
 * asks. The ability that implements it comes from a UKzCapabilitySet and is an implementation detail:
 * granting both from one place is what keeps the tag and the ability from drifting apart.
 *
 * Networking: granting is authority-only. The tag replicates to everyone and ability specs replicate to
 * the owner, so HasCapability answers correctly everywhere without this component replicating anything.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzCapabilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzCapabilityComponent();

	/** Where capabilities are looked up, and what is granted at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capabilities")
	TObjectPtr<UKzCapabilitySet> CapabilitySet;

	/** Grants every capability of CapabilitySet. Called on BeginPlay, safe to call again. */
	UFUNCTION(BlueprintCallable, Category = "Capabilities")
	void GrantDefaultCapabilities();

	/**
	 * Grants one capability: the tag always, plus whatever implements it.
	 * @param SourceSet Where to look the implementation up. Defaults to our own CapabilitySet, so an object
	 *                  handing out a temporary capability can pass its own set instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Capabilities")
	void GrantCapability(FGameplayTag Capability, UKzCapabilitySet* SourceSet = nullptr);

	/** Takes back a capability and whatever was granted with it. */
	UFUNCTION(BlueprintCallable, Category = "Capabilities")
	void RevokeCapability(FGameplayTag Capability);

	/** Whether the owner can do this. The question the world asks, and a plain tag check. */
	UFUNCTION(BlueprintPure, Category = "Capabilities")
	bool HasCapability(FGameplayTag Capability) const;

	/**
	 * Whether the owner could put a capability to use right now, which is a stronger question than having
	 * it: whatever implements it may be on cooldown, unaffordable, or blocked by the owner's current state.
	 * A capability with nothing behind it is always usable, and one already in use answers yes, so an
	 * ability that blocks itself while it runs does not report false for its own duration.
	 */
	UFUNCTION(BlueprintPure, Category = "Capabilities")
	bool CanUseCapability(FGameplayTag Capability) const;

	/**
	 * Holds a capability back without taking it away: the owner still has it, it just cannot be used
	 * while the source says so. For rules that come from the world rather than from the owner, like a
	 * room where nobody draws a weapon.
	 *
	 * Keyed by source so several can suppress the same thing and release in any order. Reaches
	 * capabilities with no ability behind them, which is what ActivationBlockedTags cannot do.
	 */
	UFUNCTION(BlueprintCallable, Category = "Capabilities")
	void SuppressCapability(FGameplayTag Capability, FName SourceID);

	/** Lifts one source's hold. The capability comes back when the last source lets go. */
	UFUNCTION(BlueprintCallable, Category = "Capabilities")
	void ReleaseCapability(FGameplayTag Capability, FName SourceID);

	/** Whether anything is currently holding this capability back. */
	UFUNCTION(BlueprintPure, Category = "Capabilities")
	bool IsCapabilitySuppressed(FGameplayTag Capability) const;

	/**
	 * Activates whatever implements a capability.
	 * Use this over a gameplay event when the caller needs to know whether anything answered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Capabilities")
	bool TryActivateCapability(FGameplayTag Capability);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Abilities we granted, so we can take back exactly what we gave. Authority only. */
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> GrantedAbilities;

	/** Who is holding what back. A capability with no sources left is free again. */
	TMap<FGameplayTag, TSet<FName>> SuppressedCapabilities;

	UAbilitySystemComponent* GetAbilitySystem() const;

	bool HasAuthority() const;
};
