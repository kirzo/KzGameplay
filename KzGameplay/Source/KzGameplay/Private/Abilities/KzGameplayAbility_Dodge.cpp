// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatTags.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"

UKzGameplayAbility_Dodge::UKzGameplayAbility_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(KzTags::Combat::State_Dead);
}

void UKzGameplayAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector Input = Pawn->GetLastMovementInputVector().GetSafeNormal2D();
	if (bFaceInput && !Input.IsNearlyZero())
	{
		Pawn->SetActorRotation(FRotator(0.0, Input.Rotation().Yaw, 0.0));
	}

	if (UKzInputHandlerComponent* InputHandler = Pawn->FindComponentByClass<UKzInputHandlerComponent>())
	{
		InputHandler->PushInputIgnore(KzTags::Input::Move, GetFName(), true, 0);
		IgnoredInput = InputHandler;
	}

	// Left at walking pace when the move ends, rather than sliding on at the dodge's
	const UCharacterMovementComponent* Movement = Pawn->FindComponentByClass<UCharacterMovementComponent>();
	UAbilityTask_ApplyRootMotionConstantForce* Move = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(this, NAME_None, Pawn->GetActorForwardVector(),
		Distance / Duration, Duration, false, SpeedCurve, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, Movement ? Movement->GetMaxSpeed() : 0.0f, true);
	Move->OnFinish.AddDynamic(this, &UKzGameplayAbility_Dodge::OnMoveFinished);
	Move->ReadyForActivation();

	FKzAnimationContext Context;
	Context.Mesh = ActorInfo->SkeletalMeshComponent.Get();
	Context.User = Pawn;
	Play = Animation ? Animation->Play(Context) : nullptr;
	if (Play)
	{
		Play->OnBlendOut.AddUObject(this, &UKzGameplayAbility_Dodge::OnAnimationBlendOut);
		Play->OnEnded.AddUObject(this, &UKzGameplayAbility_Dodge::OnAnimationEnded);
	}
}

void UKzGameplayAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
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

void UKzGameplayAbility_Dodge::OnAnimationBlendOut()
{
	K2_EndAbility();
}

void UKzGameplayAbility_Dodge::OnAnimationEnded(bool bInterrupted)
{
	K2_EndAbility();
}

void UKzGameplayAbility_Dodge::OnMoveFinished()
{
	if (!Play)
	{
		K2_EndAbility();
	}
}
