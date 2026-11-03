// Copyright 2026 kirzo

#include "Combat/KzCombatNotifies.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/KzCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"

namespace
{
	UKzInputHandlerComponent* FindInputHandler(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	}
}

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

UKzAnimNotifyState_MoveWindow::UKzAnimNotifyState_MoveWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(60, 180, 180);
#endif
}

void UKzAnimNotifyState_MoveWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	// Only in game: a preview posing the animation would walk its character away
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	const FAnimNotifyEvent* Event = EventReference.GetNotify();
	const float Duration = Event ? Event->GetDuration() : 0.0f;
	if (!Owner || !Owner->GetWorld() || !Owner->GetWorld()->IsGameWorld() || Duration <= 0.0f)
	{
		return;
	}

	// In animation time, so a hit stop holds the step and the play rate does not change its length
	UKzCombatLibrary::MoveBy(*Owner, Owner->GetActorForwardVector() * (Distance * FrameDeltaTime / Duration));
}

FString UKzAnimNotifyState_MoveWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Move %.0f"), Distance);
}

UKzAnimNotifyState_InputBlockWindow::UKzAnimNotifyState_InputBlockWindow()
{
	Inputs.AddTag(KzTags::Input::Move);
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(130, 130, 130);
#endif
}

void UKzAnimNotifyState_InputBlockWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UKzInputHandlerComponent* InputHandler = FindInputHandler(MeshComp))
	{
		for (const FGameplayTag& Input : Inputs)
		{
			InputHandler->PushInputIgnore(Input, GetSourceID(), true, 0);
		}
	}
}

void UKzAnimNotifyState_InputBlockWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UKzInputHandlerComponent* InputHandler = FindInputHandler(MeshComp))
	{
		for (const FGameplayTag& Input : Inputs)
		{
			InputHandler->RemoveInputIgnore(Input, GetSourceID());
		}
	}
}

FString UKzAnimNotifyState_InputBlockWindow::GetNotifyName_Implementation() const
{
	return Inputs.IsEmpty() ? Super::GetNotifyName_Implementation() : FString::Printf(TEXT("Block %s"), *Inputs.ToStringSimple());
}

FName UKzAnimNotifyState_InputBlockWindow::GetSourceID() const
{
	return FName(*FString::Printf(TEXT("InputBlockWindow_%u"), GetUniqueID()));
}