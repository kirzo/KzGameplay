// Copyright 2026 kirzo

#include "Combat/KzGameplayCueNotify_Burst.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/KzCombatTags.h"
#include "Combat/KzMeshShakeSubsystem.h"

bool UKzGameplayCueNotify_Burst::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	const bool bResult = Super::OnExecute_Implementation(Target, Parameters);
	if (MeshShakeAmplitude <= 0.0f || !Target || Target == Parameters.Instigator.Get())
	{
		return bResult;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (ASC && ASC->HasMatchingGameplayTag(KzTags::Combat::State_SuperArmor))
	{
		return bResult;
	}

	// The blow travels from its context's origin to the impact; without one, into the surface hit
	const FGameplayEffectContextHandle& Context = Parameters.EffectContext;
	const FVector Direction = Context.IsValid() && Context.HasOrigin() ? Parameters.Location - Context.GetOrigin() : -Parameters.Normal;
	UKzMeshShakeSubsystem::ShakeMesh(Target, Direction.GetSafeNormal(), MeshShakeAmplitude, MeshShakeDuration, MeshShakeFrequency);
	return bResult;
}