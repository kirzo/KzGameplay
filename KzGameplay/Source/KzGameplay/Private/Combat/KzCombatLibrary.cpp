// Copyright 2026 kirzo

#include "Combat/KzCombatLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/KzCombatTags.h"
#include "Equipment/KzEquipmentComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameplayEffect.h"
#include "Items/Fragments/KzItemFragment_Defense.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Items/KzItemDefinition.h"

namespace
{
	/** The equipped Defense item whose angle, as GetAngle reads it, is widest, if From lies within it. */
	bool FindDefense(const AActor& Target, const FVector& From, TFunctionRef<float(const UKzItemFragment_Defense&)> GetAngle, FKzHitDefense& OutDefense)
	{
		const UKzEquipmentComponent* Equipment = Target.FindComponentByClass<UKzEquipmentComponent>();
		if (!Equipment)
		{
			return false;
		}

		FKzHitDefense Best;
		float BestAngle = 0.0f;
		for (const FEquippedSlot& Slot : Equipment->GetSlots())
		{
			const UKzItemDefinition* Item = Slot.Instance.IsValid() ? Slot.Instance.ItemDef.Get() : nullptr;
			const UKzItemFragment_Defense* Defense = Item ? Item->FindFragmentByClass<UKzItemFragment_Defense>() : nullptr;
			const UKzItemFragment_Equippable* Equippable = Item ? Item->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
			// Put away, it defends nothing
			const float Angle = Defense && !(Equippable && Equipment->IsStowed(Slot, *Equippable)) ? GetAngle(*Defense) : 0.0f;
			if (Angle > BestAngle)
			{
				BestAngle = Angle;
				Best.Item = Item;
				Best.Component = Slot.Instance.SpawnedActor ? Slot.Instance.SpawnedActor->GetRootComponent() : Slot.Instance.SpawnedComponent.Get();
			}
		}

		if (BestAngle <= 0.0f || !UKzCombatLibrary::IsInFront(Target, From, BestAngle))
		{
			return false;
		}
		OutDefense = Best;
		return true;
	}

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

	/** The attack's cue, at the point it hit. */
	void ExecuteImpactCue(UAbilitySystemComponent* ASC, AActor* Attacker, const FKzComboHit& Hit, float Damage, const UObject* SourceObject)
	{
		if (!ASC || !Hit.ImpactCue.IsValid())
		{
			return;
		}

		FGameplayCueParameters Parameters;
		Parameters.Location = Hit.Hit.ImpactPoint;
		Parameters.Normal = Hit.Hit.ImpactNormal;
		Parameters.Instigator = Attacker;
		Parameters.EffectCauser = Attacker;
		Parameters.SourceObject = SourceObject;
		Parameters.PhysicalMaterial = Hit.Hit.PhysMaterial.Get();
		Parameters.RawMagnitude = Damage;
		Parameters.AggregatedSourceTags = Hit.AttackTags;
		ASC->ExecuteGameplayCue(Hit.ImpactCue, Parameters);
	}

	/** A defense cue, on the defending item: no location, so the cue spawns at the item's socket rather than where the hitbox touched the body. */
	void ExecuteDefenseCue(UAbilitySystemComponent* ASC, FGameplayTag Cue, AActor* Attacker, const FKzComboHit& Hit, float Damage, const UObject* SourceObject)
	{
		if (!ASC || !Cue.IsValid())
		{
			return;
		}

		FGameplayCueParameters Parameters;
		Parameters.TargetAttachComponent = Hit.Defense.Component.Get();
		Parameters.Instigator = Attacker;
		Parameters.EffectCauser = Attacker;
		Parameters.SourceObject = SourceObject;
		Parameters.RawMagnitude = Damage;
		Parameters.AggregatedSourceTags = Hit.AttackTags;
		ASC->ExecuteGameplayCue(Cue, Parameters);
	}
}

EKzHitResult UKzCombatLibrary::ResolveHit(const AActor* Attacker, const AActor* Target, const FGameplayTagContainer& AttackTags, FKzHitDefense& OutDefense)
{
	OutDefense = FKzHitDefense();
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

	const FVector From = Attacker->GetActorLocation();
	const bool bUnblockable = AttackTags.HasTag(Attack_Unblockable);
	if (!bUnblockable && ASC->HasMatchingGameplayTag(State_Parrying) && FindDefense(*Target, From, [](const UKzItemFragment_Defense& Defense) { return Defense.ParryAngle; }, OutDefense))
	{
		return EKzHitResult::Parried;
	}
	if (!bUnblockable && ASC->HasMatchingGameplayTag(State_Blocking) && FindDefense(*Target, From, [](const UKzItemFragment_Defense& Defense) { return Defense.BlockAngle; }, OutDefense))
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
	const UKzItemFragment_Defense* Defense = Hit.Defense.Item ? Hit.Defense.Item->FindFragmentByClass<UKzItemFragment_Defense>() : nullptr;
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
				Spec.Data->AppendDynamicAssetTags(Hit.AttackTags);
				AttackerASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			}
		}
		SendReaction(Target, Attacker, Hit.Reaction.IsValid() ? Hit.Reaction : Reaction_Hit.GetTag(), Hit, Damage, SourceObject);
		// Targets without an ability system still show the impact, through the attacker's.
		ExecuteImpactCue(TargetASC ? TargetASC : AttackerASC, Attacker, Hit, Damage, SourceObject);
		break;

	case EKzHitResult::Blocked:
		SendReaction(Target, Attacker, Reaction_Block, Hit, Damage, SourceObject);
		SendReaction(Attacker, Target, Reaction_Blocked, Hit, Damage, SourceObject);
		ExecuteDefenseCue(TargetASC, Defense ? Defense->BlockCue : FGameplayTag(), Attacker, Hit, Damage, SourceObject);
		break;

	case EKzHitResult::GuardBroken:
		SendReaction(Target, Attacker, Reaction_GuardBreak, Hit, Damage, SourceObject);
		ExecuteDefenseCue(TargetASC, Defense ? (Defense->GuardBreakCue.IsValid() ? Defense->GuardBreakCue : Defense->BlockCue) : FGameplayTag(), Attacker, Hit, Damage, SourceObject);
		break;

	case EKzHitResult::Parried:
		SendReaction(Attacker, Target, Reaction_Parried, Hit, Damage, SourceObject);
		ExecuteDefenseCue(TargetASC, Defense ? Defense->ParryCue : FGameplayTag(), Attacker, Hit, Damage, SourceObject);
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

void UKzCombatLibrary::MoveBy(AActor& Actor, const FVector& Delta)
{
	const APawn* Pawn = Cast<APawn>(&Actor);
	UPawnMovementComponent* Movement = Pawn ? Pawn->GetMovementComponent() : nullptr;
	if (Movement && Movement->UpdatedComponent)
	{
		FHitResult Blocked;
		Movement->SafeMoveUpdatedComponent(Delta, Movement->UpdatedComponent->GetComponentQuat(), true, Blocked);
		if (Blocked.IsValidBlockingHit())
		{
			Movement->SlideAlongSurface(Delta, 1.0f - Blocked.Time, Blocked.Normal, Blocked);
		}
	}
	else
	{
		Actor.AddActorWorldOffset(Delta, true);
	}
}