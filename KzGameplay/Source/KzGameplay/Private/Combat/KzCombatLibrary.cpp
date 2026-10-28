// Copyright 2026 kirzo

#include "Combat/KzCombatLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/KzCombatSettings.h"
#include "Combat/KzCombatTags.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"

namespace
{
	void SendReaction(AActor* Reactor, AActor* Instigator, FGameplayTag Reaction, const FKzComboHit& Hit, float Damage, const UObject* SourceObject)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Reactor);
		if (!ASC)
		{
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = Reaction;
		Payload.Instigator = Instigator;
		Payload.Target = Reactor;
		Payload.OptionalObject = SourceObject;
		Payload.InstigatorTags = Hit.AttackTags;
		Payload.EventMagnitude = Damage;
		Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Hit.Hit);

		FScopedPredictionWindow PredictionWindow(ASC, true);
		ASC->HandleGameplayEvent(Reaction, &Payload);
	}

	void ExecuteCue(UAbilitySystemComponent* ASC, FGameplayTag Cue, AActor* Instigator, const FKzComboHit& Hit, const UObject* SourceObject)
	{
		if (!ASC || !Cue.IsValid())
		{
			return;
		}

		FGameplayCueParameters Parameters;
		Parameters.Location = Hit.Hit.ImpactPoint;
		Parameters.Normal = Hit.Hit.ImpactNormal;
		Parameters.Instigator = Instigator;
		Parameters.EffectCauser = Instigator;
		Parameters.SourceObject = SourceObject;
		Parameters.PhysicalMaterial = Hit.Hit.PhysMaterial.Get();
		ASC->ExecuteGameplayCue(Cue, Parameters);
	}
}

EKzHitResult UKzCombatLibrary::ResolveHit(const AActor* Attacker, const AActor* Target, const FGameplayTagContainer& AttackTags)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!ASC || !Attacker)
	{
		return EKzHitResult::Hit;
	}

	using namespace KzTags::Combat;
	if (ASC->HasMatchingGameplayTag(State_Invulnerable) && !AttackTags.HasTag(Attack_Undodgeable))
	{
		return EKzHitResult::Ignored;
	}

	const UKzCombatSettings* Settings = GetDefault<UKzCombatSettings>();
	const FVector From = Attacker->GetActorLocation();
	const bool bUnblockable = AttackTags.HasTag(Attack_Unblockable);
	if (!bUnblockable && ASC->HasMatchingGameplayTag(State_Parrying) && IsInFront(*Target, From, Settings->ParryAngle))
	{
		return EKzHitResult::Parried;
	}
	if (!bUnblockable && ASC->HasMatchingGameplayTag(State_Blocking) && IsInFront(*Target, From, Settings->BlockAngle))
	{
		return AttackTags.HasTag(Attack_GuardBreak) ? EKzHitResult::GuardBroken : EKzHitResult::Blocked;
	}
	return EKzHitResult::Hit;
}

void UKzCombatLibrary::ApplyComboHit(AActor* Attacker, const FKzComboHit& Hit, TSubclassOf<UGameplayEffect> DamageEffect, float BaseDamage, const UObject* SourceObject)
{
	AActor* Target = Hit.Hit.GetActor();
	if (!Attacker || !Target)
	{
		return;
	}

	using namespace KzTags::Combat;
	UAbilitySystemComponent* AttackerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Attacker);
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	const UKzCombatSettings* Settings = GetDefault<UKzCombatSettings>();
	const float Damage = BaseDamage * Hit.DamageMultiplier;

	switch (Hit.Result)
	{
	case EKzHitResult::Hit:
		if (AttackerASC && TargetASC && DamageEffect)
		{
			FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
			Context.AddHitResult(Hit.Hit);
			Context.AddSourceObject(SourceObject);
			const FGameplayEffectSpecHandle Spec = AttackerASC->MakeOutgoingSpec(DamageEffect, 1.0f, Context);
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(Data_Damage, Damage);
				AttackerASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			}
		}
		SendReaction(Target, Attacker, Hit.Reaction.IsValid() ? Hit.Reaction : Reaction_Hit.GetTag(), Hit, Damage, SourceObject);
		// Targets without an ability system still show the impact, through the attacker's.
		ExecuteCue(TargetASC ? TargetASC : AttackerASC, Hit.ImpactCue, Attacker, Hit, SourceObject);
		break;

	case EKzHitResult::Blocked:
	case EKzHitResult::GuardBroken:
		SendReaction(Target, Attacker, Hit.Result == EKzHitResult::Blocked ? Reaction_Block.GetTag() : Reaction_GuardBreak.GetTag(), Hit, Damage, SourceObject);
		ExecuteCue(TargetASC, Settings->BlockCue, Attacker, Hit, SourceObject);
		break;

	case EKzHitResult::Parried:
		SendReaction(Attacker, Target, Reaction_Parried, Hit, Damage, SourceObject);
		ExecuteCue(TargetASC, Settings->ParryCue, Attacker, Hit, SourceObject);
		break;

	default:
		break;
	}
}

FGameplayTag UKzCombatLibrary::GetDirectionTag(const AActor* Actor, const FVector& From)
{
	if (!Actor)
	{
		return FGameplayTag();
	}

	using namespace KzTags::Combat;
	const FVector Local = Actor->GetActorTransform().InverseTransformVectorNoScale(From - Actor->GetActorLocation());
	if (FMath::Abs(Local.X) >= FMath::Abs(Local.Y))
	{
		return Local.X >= 0.0 ? Direction_Front.GetTag() : Direction_Back.GetTag();
	}
	return Local.Y >= 0.0 ? Direction_Right.GetTag() : Direction_Left.GetTag();
}

bool UKzCombatLibrary::IsInFront(const AActor& Actor, const FVector& From, float Angle)
{
	const FVector ToFrom = (From - Actor.GetActorLocation()).GetSafeNormal2D();
	if (ToFrom.IsNearlyZero())
	{
		return true;
	}
	const FVector Forward = Actor.GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Forward, ToFrom) >= FMath::Cos(FMath::DegreesToRadians(Angle * 0.5f));
}