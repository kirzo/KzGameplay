// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Animation.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatTags.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"

UKzGameplayAbility_Animation::UKzGameplayAbility_Animation()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(KzTags::Combat::State_Dead);
}

void UKzGameplayAbility_Animation::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FKzAnimationContext Context;
	Context.Mesh = ActorInfo->SkeletalMeshComponent.Get();
	Context.User = Avatar;
	Play = Animation ? Animation->Play(Context) : nullptr;
	if (!Play)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	Play->OnBlendOut.AddUObject(this, &UKzGameplayAbility_Animation::OnAnimationBlendOut);
	Play->OnEnded.AddUObject(this, &UKzGameplayAbility_Animation::OnAnimationEnded);

	UKzInputHandlerComponent* InputHandler = bIgnoreMoveInput ? Avatar->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	if (InputHandler)
	{
		InputHandler->PushInputIgnore(KzTags::Input::Move, GetFName(), true, 0);
		IgnoredInput = InputHandler;
	}
}

void UKzGameplayAbility_Animation::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (Play)
	{
		Play->OnBlendOut.RemoveAll(this);
		Play->OnEnded.RemoveAll(this);
		if (bWasCancelled)
		{
			Play->Stop(0.2f);
		}
		Play.Reset();
	}

	if (UKzInputHandlerComponent* InputHandler = IgnoredInput.Get())
	{
		InputHandler->RemoveInputIgnore(KzTags::Input::Move, GetFName());
	}
	IgnoredInput.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKzGameplayAbility_Animation::OnAnimationBlendOut()
{
	K2_EndAbility();
}

void UKzGameplayAbility_Animation::OnAnimationEnded(bool bInterrupted)
{
	K2_EndAbility();
}