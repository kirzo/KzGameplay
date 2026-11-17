// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Animation.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatNotifies.h"
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

	const FVector Input = bFaceInput ? GetMoveInput().GetSafeNormal2D() : FVector::ZeroVector;
	if (!Input.IsNearlyZero())
	{
		const FRotator Current = Avatar->GetActorRotation();
		Avatar->SetActorRotation(FRotator(Current.Pitch, Input.Rotation().Yaw, Current.Roll));
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
	CancelWindows = 0;
	SetCancelBlocked(CancelableBy, true);
	Play->OnNotify.AddUObject(this, &UKzGameplayAbility_Animation::OnAnimationNotify);
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
		Play->OnNotify.RemoveAll(this);
		Play->OnBlendOut.RemoveAll(this);
		Play->OnEnded.RemoveAll(this);
		if (bWasCancelled)
		{
			Play->Stop(0.2f);
		}
		Play.Reset();
	}
	SetCancelBlocked(CancelableBy, false);

	if (UKzInputHandlerComponent* InputHandler = IgnoredInput.Get())
	{
		InputHandler->RemoveInputIgnore(KzTags::Input::Move, GetFName());
	}
	IgnoredInput.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKzGameplayAbility_Animation::OnAnimationNotify(const UKzAnimationNotifyState* Notify, bool bBegin, float Duration)
{
	const UKzAnimNotifyState_CancelWindow* Window = Cast<UKzAnimNotifyState_CancelWindow>(Notify);
	if (Window && Window->bAbilities)
	{
		CancelWindows = FMath::Max(CancelWindows + (bBegin ? 1 : -1), 0);
		SetCancelBlocked(CancelableBy, CancelWindows == 0);
	}
}

void UKzGameplayAbility_Animation::OnAnimationBlendOut()
{
	K2_EndAbility();
}

void UKzGameplayAbility_Animation::OnAnimationEnded(bool bInterrupted)
{
	K2_EndAbility();
}