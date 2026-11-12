// Copyright 2026 kirzo

#include "Combat/KzCombatNotifies.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/KzCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"
#include "UObject/ObjectKey.h"

namespace
{
	UKzInputHandlerComponent* FindInputHandler(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	}

	FString GetLeaf(const FGameplayTag& Tag)
	{
		FString Leaf;
		return Tag.ToString().Split(TEXT("."), nullptr, &Leaf, ESearchCase::CaseSensitive, ESearchDir::FromEnd) ? Leaf : Tag.ToString();
	}

	/** Per owner and tag, the removing Tag Windows open on it, and how much of the tag's count the first took, so overlapping ones give it back only once the last closes. */
	struct FKzTagRemoval
	{
		int32 Windows = 0;
		int32 Taken = 0;
	};
	TMap<TObjectKey<UAbilitySystemComponent>, TMap<FGameplayTag, FKzTagRemoval>> TagRemovals;
}

UKzAnimNotifyState_HitWindow::UKzAnimNotifyState_HitWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 60, 50);
#endif
}

FString UKzAnimNotifyState_HitWindow::GetNotifyName_Implementation() const
{
	TArray<FString> Parts;
	if (!FMath::IsNearlyEqual(DamageMultiplier, 1.0f))
	{
		Parts.Add(TEXT("x") + FString::SanitizeFloat(DamageMultiplier));
	}
	for (const FGameplayTag& Tag : AttackTags)
	{
		Parts.Add(GetLeaf(Tag));
	}
	return Parts.IsEmpty() ? Super::GetNotifyName_Implementation() : TEXT("Hit Window ") + FString::Join(Parts, TEXT(" "));
}

UKzAnimNotifyState_ComboWindow::UKzAnimNotifyState_ComboWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 180, 90);
#endif
}

FString UKzAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return Window.IsValid() ? TEXT("Combo Window: ") + GetLeaf(Window) : Super::GetNotifyName_Implementation();
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
	UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner()) : nullptr;
	if (!ASC)
	{
		return;
	}
	if (!bRemove)
	{
		ASC->AddLooseGameplayTags(Tags);
		return;
	}

	// The whole count goes, from loose tags and effects alike, one at a time so a child tag's count stays.
	TMap<FGameplayTag, FKzTagRemoval>& Removals = TagRemovals.FindOrAdd(TObjectKey<UAbilitySystemComponent>(ASC));
	for (const FGameplayTag& Tag : Tags)
	{
		FKzTagRemoval& Removal = Removals.FindOrAdd(Tag);
		if (Removal.Windows++ > 0)
		{
			continue;
		}
		while (ASC->GetOwnedGameplayTags().HasTagExact(Tag))
		{
			ASC->RemoveLooseGameplayTag(Tag);
			Removal.Taken++;
		}
	}
}

void UKzAnimNotifyState_TagWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner()) : nullptr;
	if (!ASC)
	{
		return;
	}
	if (!bRemove)
	{
		ASC->RemoveLooseGameplayTags(Tags);
		return;
	}

	const TObjectKey<UAbilitySystemComponent> Key(ASC);
	TMap<FGameplayTag, FKzTagRemoval>* Removals = TagRemovals.Find(Key);
	for (const FGameplayTag& Tag : Tags)
	{
		FKzTagRemoval* Removal = Removals ? Removals->Find(Tag) : nullptr;
		if (!Removal || --Removal->Windows > 0)
		{
			continue;
		}
		if (Removal->Taken > 0)
		{
			ASC->AddLooseGameplayTag(Tag, Removal->Taken);
		}
		Removals->Remove(Tag);
	}
	if (Removals && Removals->IsEmpty())
	{
		TagRemovals.Remove(Key);
	}
}

FString UKzAnimNotifyState_TagWindow::GetNotifyName_Implementation() const
{
	if (Tags.IsEmpty())
	{
		return Super::GetNotifyName_Implementation();
	}
	return bRemove ? TEXT("Remove ") + Tags.ToStringSimple() : Tags.ToStringSimple();
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

UKzAnimNotifyState_RotationWindow::UKzAnimNotifyState_RotationWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 110, 170);
#endif
}

void UKzAnimNotifyState_RotationWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	const UKzInputHandlerComponent* InputHandler = FindInputHandler(MeshComp);
	if (!Owner || !InputHandler || !Owner->GetWorld() || !Owner->GetWorld()->IsGameWorld())
	{
		return;
	}

	const FVector Direction = InputHandler->GetIntendedInput(KzTags::Input::Move).GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// In animation time, like the Move Window
	const FRotator Current = Owner->GetActorRotation();
	Owner->SetActorRotation(FRotator(Current.Pitch, FMath::FixedTurn(Current.Yaw, Direction.Rotation().Yaw, TurnRate * FrameDeltaTime), Current.Roll));
}

FString UKzAnimNotifyState_RotationWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Rotate %.0f"), TurnRate);
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