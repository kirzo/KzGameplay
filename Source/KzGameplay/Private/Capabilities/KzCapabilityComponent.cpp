// Copyright 2026 kirzo

#include "Capabilities/KzCapabilityComponent.h"
#include "Capabilities/KzCapabilitySet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

UKzCapabilityComponent::UKzCapabilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzCapabilityComponent::BeginPlay()
{
	Super::BeginPlay();

	GrantDefaultCapabilities();
}

void UKzCapabilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		TArray<FGameplayTag> Granted;
		GrantedAbilities.GetKeys(Granted);

		for (const FGameplayTag& Capability : Granted)
		{
			RevokeCapability(Capability);
		}
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UKzCapabilityComponent::GetAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

bool UKzCapabilityComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void UKzCapabilityComponent::GrantDefaultCapabilities()
{
	if (!CapabilitySet || !HasAuthority())
	{
		return;
	}

	TMap<FGameplayTag, FKzCapabilityGrant> Resolved;
	CapabilitySet->CollectCapabilities(Resolved);

	for (const TPair<FGameplayTag, FKzCapabilityGrant>& Pair : Resolved)
	{
		GrantCapability(Pair.Key, CapabilitySet);
	}
}

void UKzCapabilityComponent::GrantCapability(FGameplayTag Capability, UKzCapabilitySet* SourceSet)
{
	if (!Capability.IsValid() || !HasAuthority() || HasCapability(Capability))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s cannot grant %s without an ability system."), *GetNameSafe(GetOwner()), *Capability.ToString());
		return;
	}

	// The tag goes on regardless: a capability with nothing behind it still means the owner can do the thing
	ASC->AddLooseGameplayTag(Capability, 1, EGameplayTagReplicationState::TagOnly);

	const UKzCapabilitySet* Set = SourceSet ? SourceSet : CapabilitySet.Get();
	const FKzCapabilityGrant* Grant = Set ? Set->FindGrant(Capability) : nullptr;

	if (Grant && Grant->Ability)
	{
		FGameplayAbilitySpec Spec(Grant->Ability, Grant->Level, INDEX_NONE, this);
		GrantedAbilities.Add(Capability, ASC->GiveAbility(Spec));
	}
}

void UKzCapabilityComponent::RevokeCapability(FGameplayTag Capability)
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC)
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(Capability, 1, EGameplayTagReplicationState::TagOnly);

	FGameplayAbilitySpecHandle Handle;
	if (GrantedAbilities.RemoveAndCopyValue(Capability, Handle))
	{
		ASC->ClearAbility(Handle);
	}
}

bool UKzCapabilityComponent::HasCapability(FGameplayTag Capability) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystem();
	return ASC && ASC->HasMatchingGameplayTag(Capability);
}

bool UKzCapabilityComponent::TryActivateCapability(FGameplayTag Capability)
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC || !HasCapability(Capability))
	{
		return false;
	}

	// Only the authority tracks handles, so everyone else looks the ability up through the set
	if (const FGameplayAbilitySpecHandle* Handle = GrantedAbilities.Find(Capability))
	{
		return ASC->TryActivateAbility(*Handle);
	}

	const FKzCapabilityGrant* Grant = CapabilitySet ? CapabilitySet->FindGrant(Capability) : nullptr;
	if (!Grant || !Grant->Ability)
	{
		return false;
	}

	return ASC->TryActivateAbilityByClass(Grant->Ability);
}
