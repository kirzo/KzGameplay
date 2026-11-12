// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_HitReaction.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/AnyOf.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatTags.h"
#include "Combat/KzCombatTypes.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

namespace
{
	// A blow with less of itself along the ground comes mostly from above or below.
	constexpr float MinGroundPart = 0.2f;
}

UKzGameplayAbility_HitReaction::UKzGameplayAbility_HitReaction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
	ActivationBlockedTags.AddTag(KzTags::Combat::State_SuperArmor);
	ActivationBlockedTags.AddTag(KzTags::Combat::State_Dead);

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = KzTags::Combat::Reaction;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	Directions.Add(KzTags::Combat::Direction_Front, FVector::ForwardVector);
	Directions.Add(KzTags::Combat::Direction_Back, FVector::BackwardVector);
	Directions.Add(KzTags::Combat::Direction_Right, FVector::RightVector);
	Directions.Add(KzTags::Combat::Direction_Left, FVector::LeftVector);
}

bool UKzGameplayAbility_HitReaction::FindKnockback(const AActor* Avatar, FVector& OutDirection, float& OutScale)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	if (!ASC)
	{
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UKzGameplayAbility_HitReaction* Reaction = Spec.IsActive() ? Cast<UKzGameplayAbility_HitReaction>(Spec.GetPrimaryInstance()) : nullptr;
		if (Reaction)
		{
			OutDirection = Reaction->KnockbackDirection;
			OutScale = Reaction->Knockback;
			return true;
		}
	}
	return false;
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
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	const FGameplayAbilityTargetData* Data = TriggerEventData ? TriggerEventData->TargetData.Get(0) : nullptr;
	const FKzHitReactionTargetData* Hit = Data && Data->GetScriptStruct()->IsChildOf(FKzHitReactionTargetData::StaticStruct()) ? static_cast<const FKzHitReactionTargetData*>(Data) : nullptr;

	// Without a direction of its own, the blow comes straight from the instigator.
	const FVector FromInstigator = Avatar && Instigator ? (Avatar->GetActorLocation() - Instigator->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
	const FVector Blow = Hit && !Hit->Direction.IsNearlyZero() ? Hit->Direction : FromInstigator;
	Knockback = Hit ? Hit->Knockback : 1.0f;
	KnockbackDirection = Blow.Size2D() >= MinGroundPart ? Blow.GetSafeNormal2D() : FromInstigator;
	if (KnockbackDirection.IsNearlyZero() && Avatar)
	{
		KnockbackDirection = -Avatar->GetActorForwardVector();
	}

	FKzAnimationContext Context;
	Context.Mesh = ActorInfo->SkeletalMeshComponent.Get();
	Context.User = Avatar;
	Context.Tags.AddTag(Reaction);
	if (TriggerEventData)
	{
		Context.Tags.AppendTags(TriggerEventData->InstigatorTags);
	}
	if (Avatar)
	{
		Context.Tags.AddTag(FaceBlow(*Avatar, Blow, FromInstigator));
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TurnTimer);
	}

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

FGameplayTag UKzGameplayAbility_HitReaction::FaceBlow(AActor& Avatar, const FVector& Blow, const FVector& FromInstigator)
{
	// Where the blow comes from: along the ground, unless a side faces up or down.
	const bool bVertical = Algo::AnyOf(Directions, [](const TPair<FGameplayTag, FVector>& Side) { return !FMath::IsNearlyZero(Side.Value.GetSafeNormal().Z); });
	FVector Incoming = -Blow.GetSafeNormal();
	if (!bVertical)
	{
		Incoming = Blow.Size2D() >= MinGroundPart ? -Blow.GetSafeNormal2D() : -FromInstigator;
	}
	if (Incoming.IsNearlyZero())
	{
		Incoming = Avatar.GetActorForwardVector();
	}

	const FVector Local = Avatar.GetActorTransform().InverseTransformVectorNoScale(Incoming);
	const TPair<FGameplayTag, FVector>* Best = nullptr;
	float BestDot = -2.0f;
	for (const TPair<FGameplayTag, FVector>& Side : Directions)
	{
		const float Dot = FVector::DotProduct(Local, Side.Value.GetSafeNormal());
		if (Dot > BestDot)
		{
			BestDot = Dot;
			Best = &Side;
		}
	}
	if (!Best)
	{
		return FGameplayTag();
	}

	// The turn that has the blow come in along that side's way, at most MaxTurn.
	const FVector SideGround(Best->Value.X, Best->Value.Y, 0.0);
	const FVector IncomingGround(Incoming.X, Incoming.Y, 0.0);
	if (MaxTurn > 0.0f && !SideGround.IsNearlyZero() && !IncomingGround.IsNearlyZero())
	{
		TurnFrom = Avatar.GetActorRotation().Yaw;
		TurnBy = FMath::Clamp(FMath::FindDeltaAngleDegrees(TurnFrom, IncomingGround.Rotation().Yaw - SideGround.Rotation().Yaw), -MaxTurn, MaxTurn);
		TurnElapsed = 0.0f;
		if (TurnTime > 0.0f && Avatar.GetWorld())
		{
			TurnTimer = Avatar.GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKzGameplayAbility_HitReaction::TickTurn);
		}
		else
		{
			TickTurn();
		}
	}
	return Best->Key;
}

void UKzGameplayAbility_HitReaction::TickTurn()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!Avatar || !World || !IsActive())
	{
		return;
	}

	// In the avatar's time, so a hit stop holds the turn.
	if (TurnTime > 0.0f)
	{
		TurnElapsed += World->GetDeltaSeconds() * Avatar->CustomTimeDilation;
	}
	const float Alpha = TurnTime > 0.0f ? FMath::Min(TurnElapsed / TurnTime, 1.0f) : 1.0f;

	FRotator Rotation = Avatar->GetActorRotation();
	Rotation.Yaw = TurnFrom + TurnBy * Alpha;
	Avatar->SetActorRotation(Rotation);

	if (Alpha < 1.0f)
	{
		TurnTimer = World->GetTimerManager().SetTimerForNextTick(this, &UKzGameplayAbility_HitReaction::TickTurn);
	}
}

void UKzGameplayAbility_HitReaction::OnAnimationEnded(bool bInterrupted)
{
	K2_EndAbility();
}