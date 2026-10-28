// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_HitReaction.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatLibrary.h"
#include "Combat/KzCombatTags.h"
#include "GameplayEffect.h"

UKzGameplayAbility_HitReaction::UKzGameplayAbility_HitReaction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
	ActivationBlockedTags.AddTag(KzTags::Combat::State_HyperArmor);

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = KzTags::Combat::Reaction;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UKzGameplayAbility_HitReaction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayTag Reaction = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();
	for (FGameplayTag Tag = Reaction; Tag.IsValid(); Tag = Tag.RequestDirectParent())
	{
		if (const TSubclassOf<UGameplayEffect>* Effect = Effects.Find(Tag))
		{
			BP_ApplyGameplayEffectToOwner(*Effect);
			break;
		}
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	FKzAnimationContext Context;
	Context.Mesh = ActorInfo->SkeletalMeshComponent.Get();
	Context.User = Avatar;
	Context.Tags.AddTag(Reaction);
	if (const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr)
	{
		Context.Tags.AddTag(UKzCombatLibrary::GetDirectionTag(Avatar, Instigator->GetActorLocation()));
	}

	Play = Animation && Reaction.IsValid() ? Animation->Play(Context) : nullptr;
	if (!Play)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	Play->OnEnded.AddUObject(this, &UKzGameplayAbility_HitReaction::OnAnimationEnded);
}

void UKzGameplayAbility_HitReaction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (Play)
	{
		Play->OnEnded.RemoveAll(this);
		if (bWasCancelled)
		{
			Play->Stop(0.2f);
		}
		Play.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKzGameplayAbility_HitReaction::OnAnimationEnded(bool bInterrupted)
{
	K2_EndAbility();
}