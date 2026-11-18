// Copyright 2026 kirzo

#include "Combat/KzCombatNotifies.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "Abilities/KzGameplayAbility_HitReaction.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Combat/KzCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/CollisionProfile.h"
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

	/** Per owner and tag, the removing Tag Windows open on it, and how much of the tag's count the first took, so overlapping ones give it back only once the last closes. */
	struct FKzTagRemoval
	{
		int32 Windows = 0;
		int32 Taken = 0;
	};
	TMap<TObjectKey<UAbilitySystemComponent>, TMap<FGameplayTag, FKzTagRemoval>> TagRemovals;

	/** Per mesh, how long each window with a Speed Curve has been open on it, in animation time: a montage's notifies do not say. */
	using FKzDisplacementKey = TPair<TObjectKey<USkeletalMeshComponent>, TObjectKey<UKzAnimNotifyState_Displacement>>;
	TMap<FKzDisplacementKey, float> DisplacementTimes;

	/** The area under the curve from From to To, sampled halfway through Steps equal steps. */
	float IntegrateCurve(const FRichCurve& Curve, float From, float To, int32 Steps)
	{
		const float Step = (To - From) / Steps;
		float Sum = 0.0f;
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			Sum += Curve.Eval(From + (Index + 0.5f) * Step);
		}
		return Sum * Step;
	}

	/** Per root component and channel, the Collision Windows open on it and the response the first took, to give back once the last closes. */
	struct FKzIgnoredChannel
	{
		int32 Windows = 0;
		ECollisionResponse Response = ECR_Block;
	};
	TMap<TObjectKey<UPrimitiveComponent>, TMap<ECollisionChannel, FKzIgnoredChannel>> IgnoredChannels;

	/** The mesh owner's root, such as a character's capsule, in game only: a preview has nothing to pass through. */
	UPrimitiveComponent* FindGameRoot(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		const UWorld* World = Owner ? Owner->GetWorld() : nullptr;
		return World && World->IsGameWorld() ? Cast<UPrimitiveComponent>(Owner->GetRootComponent()) : nullptr;
	}
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
	if (Direction.Mode != EKzHitDirectionMode::Preset)
	{
		Parts.Add(UEnum::GetDisplayValueAsText(Direction.Mode).ToString());
	}
	else
	{
		// One missing from Project Settings plays straight through the target: worth seeing on the timeline
		const FString Name = FName::NameToDisplayString(Direction.PresetTag.GetTagLeafName().ToString(), false);
		if (!FKzHitDirection::FindPreset(Direction.PresetTag))
		{
			Parts.Add(Name + TEXT(" (missing)"));
		}
		else if (Direction.PresetTag != KzTags::Combat::HitDirection_Thrust)
		{
			Parts.Add(Name);
		}
	}
	if (!FMath::IsNearlyEqual(DamageMultiplier, 1.0f))
	{
		Parts.Add(TEXT("x") + FString::SanitizeFloat(DamageMultiplier));
	}
	if (!FMath::IsNearlyEqual(Knockback, 1.0f))
	{
		Parts.Add(TEXT("Knockback x") + FString::SanitizeFloat(Knockback));
	}
	for (const FGameplayTag& Tag : AttackTags)
	{
		Parts.Add(Tag.GetTagLeafName().ToString());
	}
	return Parts.IsEmpty() ? Super::GetNotifyName_Implementation() : TEXT("Hit Window ") + FString::Join(Parts, TEXT(" "));
}

UKzAnimNotifyState_ComboWindow::UKzAnimNotifyState_ComboWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 180, 90);
#endif
}

float UKzAnimNotifyState_ComboWindow::GetInputBuffer() const
{
	return InputBuffer.ToSeconds(GetTypedOuter<UAnimSequenceBase>());
}

FString UKzAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return Window.IsValid() ? TEXT("Combo Window: ") + Window.GetTagLeafName().ToString() : Super::GetNotifyName_Implementation();
}

UKzAnimNotifyState_CancelWindow::UKzAnimNotifyState_CancelWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 130, 220);
#endif
}

float UKzAnimNotifyState_CancelWindow::GetInputBuffer() const
{
	return InputBuffer.ToSeconds(GetTypedOuter<UAnimSequenceBase>());
}

void UKzAnimNotifyState_CancelWindow::ReplayInput(const AActor* Owner) const
{
	if (UKzAbilitySystemComponent* ASC = Cast<UKzAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner)))
	{
		ASC->ReplayKeptInput(GetInputBuffer());
	}
}

void UKzAnimNotifyState_CancelWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	// Only in game, where there is a stick: the editors' previews would stop their own playback.
	const UKzInputHandlerComponent* InputHandler = bMovement ? FindInputHandler(MeshComp) : nullptr;
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!InputHandler || !AnimInstance || !MeshComp->GetWorld() || !MeshComp->GetWorld()->IsGameWorld())
	{
		return;
	}

	// The stick as pushed, even while Input.Move is blocked; below this, a resting stick or a light touch.
	constexpr float MinMoveInput = 0.25f;
	if (InputHandler->GetIntendedInput(KzTags::Input::Move).SizeSquared() < FMath::Square(MinMoveInput))
	{
		return;
	}

	// A notify in a sequence the montage plays reports the sequence: the montage is the one playing.
	UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (!Montage)
	{
		Montage = AnimInstance->GetCurrentActiveMontage();
	}
	if (Montage)
	{
		AnimInstance->Montage_Stop(Montage->BlendOut.GetBlendTime(), Montage);
	}
}

FString UKzAnimNotifyState_CancelWindow::GetNotifyName_Implementation() const
{
	TArray<FString> By;
	if (bAbilities)
	{
		By.Add(TEXT("Abilities"));
	}
	if (bMovement)
	{
		By.Add(TEXT("Movement"));
	}
	return By.IsEmpty() ? Super::GetNotifyName_Implementation() : TEXT("Cancel: ") + FString::Join(By, TEXT(", "));
}

float UKzAnimNotifyState_Displacement::GetShare(float From, float To) const
{
	From = FMath::Clamp(From, 0.0f, 1.0f);
	To = FMath::Clamp(To, 0.0f, 1.0f);
	const FRichCurve* Curve = SpeedCurve.GetRichCurveConst();
	if (To <= From || !Curve || Curve->IsEmpty())
	{
		return FMath::Max(To - From, 0.0f);
	}

	// Against the whole window's, so it always adds up to the whole distance
	const float Whole = IntegrateCurve(*Curve, 0.0f, 1.0f, 32);
	return Whole > UE_SMALL_NUMBER ? IntegrateCurve(*Curve, From, To, 8) / Whole : To - From;
}

float UKzAnimNotifyState_Displacement::TickShare(const USkeletalMeshComponent* MeshComp, float FrameDeltaTime, float Duration)
{
	const FRichCurve* Curve = SpeedCurve.GetRichCurveConst();
	if (!Curve || Curve->IsEmpty())
	{
		return FrameDeltaTime / Duration;
	}

	float& Elapsed = DisplacementTimes.FindOrAdd(FKzDisplacementKey(MeshComp, this));
	const float Share = GetShare(Elapsed / Duration, (Elapsed + FrameDeltaTime) / Duration);
	Elapsed += FrameDeltaTime;
	return Share;
}

void UKzAnimNotifyState_Displacement::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	DisplacementTimes.Remove(FKzDisplacementKey(MeshComp, this));
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

UKzAnimNotifyState_KnockbackWindow::UKzAnimNotifyState_KnockbackWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(90, 140, 230);
#endif
}

void UKzAnimNotifyState_KnockbackWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	// Only in game, in animation time, as the Move Window.
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	const FAnimNotifyEvent* Event = EventReference.GetNotify();
	const float Duration = Event ? Event->GetDuration() : 0.0f;
	if (!Owner || !Owner->GetWorld() || !Owner->GetWorld()->IsGameWorld() || Duration <= 0.0f)
	{
		return;
	}

	FVector Direction;
	float Scale = 1.0f;
	if (!UKzGameplayAbility_HitReaction::FindKnockback(Owner, Direction, Scale))
	{
		Direction = -Owner->GetActorForwardVector();
	}
	UKzCombatLibrary::MoveBy(*Owner, Direction * (Distance * Scale * TickShare(MeshComp, FrameDeltaTime, Duration)));
}

FString UKzAnimNotifyState_KnockbackWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Knockback %.0f"), Distance);
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
	UKzCombatLibrary::MoveBy(*Owner, Owner->GetActorForwardVector() * (Distance * TickShare(MeshComp, FrameDeltaTime, Duration)));
}

FString UKzAnimNotifyState_MoveWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Move %.0f"), Distance);
}

UKzAnimNotifyState_CollisionWindow::UKzAnimNotifyState_CollisionWindow()
{
	Channels.Add(ECC_Pawn);
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 160, 60);
#endif
}

void UKzAnimNotifyState_CollisionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UPrimitiveComponent* Root = FindGameRoot(MeshComp);
	if (!Root)
	{
		return;
	}

	TMap<ECollisionChannel, FKzIgnoredChannel>& Ignored = IgnoredChannels.FindOrAdd(Root);
	for (const TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
		FKzIgnoredChannel& Entry = Ignored.FindOrAdd(Channel.GetValue());
		if (Entry.Windows++ == 0)
		{
			Entry.Response = Root->GetCollisionResponseToChannel(Channel);
			Root->SetCollisionResponseToChannel(Channel, ECR_Ignore);
		}
	}
}

void UKzAnimNotifyState_CollisionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UPrimitiveComponent* Root = FindGameRoot(MeshComp);
	TMap<ECollisionChannel, FKzIgnoredChannel>* Ignored = Root ? IgnoredChannels.Find(Root) : nullptr;
	if (!Ignored)
	{
		return;
	}

	for (const TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
		FKzIgnoredChannel* Entry = Ignored->Find(Channel.GetValue());
		if (Entry && --Entry->Windows == 0)
		{
			Root->SetCollisionResponseToChannel(Channel, Entry->Response);
			Ignored->Remove(Channel.GetValue());
		}
	}
	if (Ignored->IsEmpty())
	{
		IgnoredChannels.Remove(Root);
	}
}

FString UKzAnimNotifyState_CollisionWindow::GetNotifyName_Implementation() const
{
	TArray<FString> Names;
	for (const TEnumAsByte<ECollisionChannel> Channel : Channels)
	{
		Names.Add(UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(Channel).ToString());
	}
	return Names.IsEmpty() ? Super::GetNotifyName_Implementation() : TEXT("Ignore ") + FString::Join(Names, TEXT(", "));
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