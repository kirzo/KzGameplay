// Copyright 2026 kirzo

#include "Capabilities/KzCapabilityComponent.h"
#include "Capabilities/KzCapabilitySet.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputProfile.h"
#include "Kismet/KzSystemLibrary.h"
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
		// The rest are defined here and handed out by whatever grants them: an item, a slot, a script
		if (Pair.Value.bGrantedByDefault)
		{
			GrantCapability(Pair.Key, CapabilitySet);
		}
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

	if (!Grant)
	{
		return;
	}

	if (Grant->Ability)
	{
		FGameplayAbilitySpec Spec(Grant->Ability, Grant->Level, INDEX_NONE, this);
		GrantedAbilities.Add(Capability, ASC->GiveAbility(Spec));
	}

	// The controls come with the capability, so nothing that grants one has to know about inputs.
	// ponytail: pushed here, which is authority-only. Fine while play is local; online this wants to
	// react to the replicated tag instead, so the owning client gets its controls too
	if (UKzInputHandlerComponent* Input = GetInputHandler())
	{
		for (UKzInputProfile* Profile : Grant->InputProfiles)
		{
			Input->PushInputProfile(Profile);
		}

		GrantedProfiles.Add(Capability, Grant->InputProfiles);
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

	TArray<TObjectPtr<UKzInputProfile>> Profiles;
	if (GrantedProfiles.RemoveAndCopyValue(Capability, Profiles))
	{
		if (UKzInputHandlerComponent* Input = GetInputHandler())
		{
			for (UKzInputProfile* Profile : Profiles)
			{
				Input->RemoveInputProfile(Profile);
			}
		}
	}
}

UKzInputHandlerComponent* UKzCapabilityComponent::GetInputHandler() const
{
	return UKzSystemLibrary::FindComponentInActorOrController<UKzInputHandlerComponent>(GetOwner());
}

bool UKzCapabilityComponent::HasCapability(FGameplayTag Capability) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystem();
	return ASC && ASC->HasMatchingGameplayTag(Capability);
}

void UKzCapabilityComponent::SuppressCapability(FGameplayTag Capability, FName SourceID)
{
	if (Capability.IsValid())
	{
		SuppressedCapabilities.FindOrAdd(Capability).Add(SourceID);
	}
}

void UKzCapabilityComponent::ReleaseCapability(FGameplayTag Capability, FName SourceID)
{
	if (TSet<FName>* Sources = SuppressedCapabilities.Find(Capability))
	{
		Sources->Remove(SourceID);

		if (Sources->IsEmpty())
		{
			SuppressedCapabilities.Remove(Capability);
		}
	}
}

bool UKzCapabilityComponent::IsCapabilitySuppressed(FGameplayTag Capability) const
{
	// ponytail: exact match. Suppressing a parent tag to hold everything under it is the obvious next
	// step, and turns this lookup into a walk over the map when somebody needs it
	return SuppressedCapabilities.Contains(Capability);
}

bool UKzCapabilityComponent::CanUseCapability(FGameplayTag Capability) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC || !HasCapability(Capability) || IsCapabilitySuppressed(Capability))
	{
		return false;
	}

	// Only the authority tracks handles, so everyone else looks the ability up through the set
	FGameplayAbilitySpec* Spec = nullptr;
	if (const FGameplayAbilitySpecHandle* Handle = GrantedAbilities.Find(Capability))
	{
		Spec = ASC->FindAbilitySpecFromHandle(*Handle);
	}
	else if (const FKzCapabilityGrant* Grant = CapabilitySet ? CapabilitySet->FindGrant(Capability) : nullptr)
	{
		Spec = Grant->Ability ? ASC->FindAbilitySpecFromClass(Grant->Ability) : nullptr;
	}

	// Nothing implements it, so there is nothing that could refuse
	if (!Spec || !Spec->Ability)
	{
		return true;
	}

	// Using it counts as being able to. Without this, an ability that blocks itself through its own
	// ActivationOwnedTags would answer no for exactly as long as it runs
	if (Spec->IsActive())
	{
		return true;
	}

	return Spec->Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get());
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
