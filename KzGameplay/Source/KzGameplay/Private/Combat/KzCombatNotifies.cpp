// Copyright 2026 kirzo

#include "Combat/KzCombatNotifies.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

UKzAnimNotifyState_HitWindow::UKzAnimNotifyState_HitWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 60, 50);
#endif
}

UKzAnimNotifyState_ComboWindow::UKzAnimNotifyState_ComboWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 180, 90);
#endif
}

UKzAnimNotifyState_CancelWindow::UKzAnimNotifyState_CancelWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 130, 220);
#endif
}

UKzAnimNotifyState_WarpWindow::UKzAnimNotifyState_WarpWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(170, 90, 210);
#endif
}

UKzAnimNotifyState_TagWindow::UKzAnimNotifyState_TagWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 170, 60);
#endif
}

void UKzAnimNotifyState_TagWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner()) : nullptr)
	{
		ASC->AddLooseGameplayTags(Tags);
	}
}

void UKzAnimNotifyState_TagWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner()) : nullptr)
	{
		ASC->RemoveLooseGameplayTags(Tags);
	}
}

FString UKzAnimNotifyState_TagWindow::GetNotifyName_Implementation() const
{
	return Tags.IsEmpty() ? Super::GetNotifyName_Implementation() : Tags.ToStringSimple();
}