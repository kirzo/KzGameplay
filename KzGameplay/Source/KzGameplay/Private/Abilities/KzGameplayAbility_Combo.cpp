// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Combo.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputTag.h"
#include "Combat/KzCombatLibrary.h"
#include "Combat/KzCombatTags.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboInstance.h"
#include "Equipment/KzEquipmentComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Items/Fragments/KzItemFragment_MeleeWeapon.h"
#include "Items/KzItemDefinition.h"

UKzGameplayAbility_Combo::UKzGameplayAbility_Combo()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationOwnedTags.AddTag(KzTags::Combat::State_Attacking);
}

void UKzGameplayAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = GetAvatarActorFromActorInfo();
	const UKzEquipmentComponent* Equipment = Avatar && WeaponSlot.IsValid() ? Avatar->FindComponentByClass<UKzEquipmentComponent>() : nullptr;
	const FKzItemInstance* Equipped = Equipment ? Equipment->FindItemInSlot(WeaponSlot) : nullptr;
	const UKzItemDefinition* ItemDef = Equipped && Equipped->IsValid() ? Equipped->ItemDef.Get() : nullptr;
	const UKzItemFragment_MeleeWeapon* Melee = ItemDef ? ItemDef->FindFragmentByClass<UKzItemFragment_MeleeWeapon>() : nullptr;

	UKzComboGraph* GrantedCombo = Cast<UKzComboGraph>(GetCurrentSourceObject());
	UKzComboGraph* Combo = Melee ? Melee->Combo.LoadSynchronous() : GrantedCombo ? GrantedCombo : UnarmedCombo.Get();
	if (!Combo || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Item = Melee ? ItemDef : nullptr;
	BaseDamage = Melee ? Melee->BaseDamage : UnarmedDamage;
	DamageEffect = Melee ? Melee->DamageEffect : UnarmedDamageEffect;

	FKzComboContext Context;
	Context.Mesh = ActorInfo->SkeletalMeshComponent.Get();
	Context.User = Avatar;
	Context.Item = Item;
	Context.ObjectTypes = ObjectTypes;
	if (Melee && Equipped)
	{
		Context.Weapon = Equipped->SpawnedActor ? Equipped->SpawnedActor->GetRootComponent() : Equipped->SpawnedComponent.Get();
	}

	// The input that activated it: the event's when triggered by one, else whichever of the combo's is held.
	FGameplayTag Input = TriggerEventData && Combo->Inputs.Contains(TriggerEventData->EventTag) ? TriggerEventData->EventTag : FGameplayTag();
	for (int32 Index = 0; !Input.IsValid() && Index < Combo->Inputs.Num(); ++Index)
	{
		if (IsInputPressed(Combo->Inputs[Index]))
		{
			Input = Combo->Inputs[Index];
		}
	}
	if (!Input.IsValid() && !Combo->Inputs.IsEmpty())
	{
		Input = Combo->Inputs[0];
	}

	SetCancelBlocked(true);
	Instance = UKzComboInstance::StartCombo(Combo, Context, Input, GetMoveInput());
	if (!Instance)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Instance->OnHit.AddDynamic(this, &UKzGameplayAbility_Combo::OnComboHit);
	Instance->OnEnded.AddDynamic(this, &UKzGameplayAbility_Combo::OnComboEnded);
	Instance->OnCancelable.AddDynamic(this, &UKzGameplayAbility_Combo::OnCancelable);

	UAbilityTask_WaitInputTag* WaitInput = UAbilityTask_WaitInputTag::WaitInputTag(this, FGameplayTagContainer::CreateFromArray(Combo->Inputs), false);
	WaitInput->OnPressed.AddDynamic(this, &UKzGameplayAbility_Combo::OnInputPressed);
	WaitInput->ReadyForActivation();
}

void UKzGameplayAbility_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (Instance)
	{
		Instance->OnHit.RemoveAll(this);
		Instance->OnEnded.RemoveAll(this);
		Instance->OnCancelable.RemoveAll(this);
		Instance->Stop();
		Instance = nullptr;
	}
	SetCancelBlocked(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKzGameplayAbility_Combo::OnInputPressed(FGameplayTag Input)
{
	if (Instance)
	{
		Instance->Press(Input, GetMoveInput());
	}
}

void UKzGameplayAbility_Combo::OnComboHit(const FKzComboHit& Hit)
{
	UKzCombatLibrary::ApplyComboHit(GetAvatarActorFromActorInfo(), Hit, DamageEffect, BaseDamage, Item);
}

void UKzGameplayAbility_Combo::OnComboEnded(bool bInterrupted)
{
	K2_EndAbility();
}

void UKzGameplayAbility_Combo::OnCancelable(bool bCancelable)
{
	SetCancelBlocked(!bCancelable);
}

void UKzGameplayAbility_Combo::SetCancelBlocked(bool bBlocked)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || CancelableBy.IsEmpty() || bCancelBlocked == bBlocked)
	{
		return;
	}

	bCancelBlocked = bBlocked;
	if (bBlocked)
	{
		ASC->BlockAbilitiesWithTags(CancelableBy);
	}
	else
	{
		ASC->UnBlockAbilitiesWithTags(CancelableBy);
	}
}