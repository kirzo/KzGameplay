// Copyright 2026 kirzo

#include "Combat/KzComboInstance.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatNotifies.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Items/KzItemDefinition.h"
#include "KzDrawDebugHelpers.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDrawHitboxes(TEXT("kz.Combo.DrawHitboxes"), false, TEXT("Draws combo hitboxes as they sweep: green, red where they hit."));

	// A swing turns what holds the hitbox this much at most between two sweeps, so the sweep follows the arc instead of cutting across it.
	constexpr float MaxStepAngle = UE_DOUBLE_PI / 18.0;
	constexpr int32 MaxSteps = 8;
}

UKzComboInstance* UKzComboInstance::StartCombo(UKzComboGraph* Combo, const FKzComboContext& Context, FGameplayTag Input)
{
	if (!Combo || !Context.Mesh)
	{
		return nullptr;
	}

	const FKzGraphData& Data = Combo->GetData();
	const int32 Entry = Data.FindEntry(FKzComboNode_Entry::EntryName);
	const int32 Output = Entry != INDEX_NONE ? Data.FindOutput(Entry, Input.GetTagName()) : INDEX_NONE;
	const TConstArrayView<FKzGraphLink> Links = Output != INDEX_NONE ? Data.GetLinks(Entry, Output) : TConstArrayView<FKzGraphLink>();
	if (Links.IsEmpty())
	{
		return nullptr;
	}

	UKzComboInstance* Instance = NewObject<UKzComboInstance>(Context.User ? static_cast<UObject*>(Context.User) : GetTransientPackage());
	Instance->Combo = Combo;
	Instance->Context = Context;
	Instance->bRunning = true;
	return Instance->PlayAttack(Links[0].Node) ? Instance : nullptr;
}

void UKzComboInstance::Press(FGameplayTag Input)
{
	if (!bRunning)
	{
		return;
	}

	if (bComboWindow)
	{
		Chain(Input);
		return;
	}

	KeptInput = Input;
	KeptAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UKzComboInstance::Stop(float BlendOut)
{
	if (Play)
	{
		Play->Stop(BlendOut);
	}
	End(true);
}

UWorld* UKzComboInstance::GetWorld() const
{
	return Context.Mesh ? Context.Mesh->GetWorld() : nullptr;
}

void UKzComboInstance::Tick(float DeltaTime)
{
	if (HitWindows > 0)
	{
		SweepHitboxes();
	}
}

TStatId UKzComboInstance::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzComboInstance, STATGROUP_Tickables);
}

bool UKzComboInstance::PlayAttack(int32 NodeIndex)
{
	const FKzComboNode_Attack* Attack = Combo->GetData().GetNode<FKzComboNode_Attack>(NodeIndex);
	FKzAnimationContext AnimationContext;
	AnimationContext.Mesh = Context.Mesh;
	AnimationContext.User = Context.User;
	AnimationContext.Item = Context.Item;

	// The attack playing gives way to it, so only the new one is heard from now on.
	const TSharedPtr<FKzAnimationPlay> NewPlay = Attack && Attack->Animation ? Attack->Animation->Play(AnimationContext) : nullptr;
	if (!NewPlay)
	{
		End(true);
		return false;
	}

	Play = NewPlay;
	Node = NodeIndex;
	bHit = false;
	bComboWindow = false;
	HitWindows = 0;
	KeptInput = FGameplayTag();
	HitActors.Reset();
	PreviousParents.Init(TOptional<FTransform>(), Attack->Hitboxes.Num());

	Play->OnNotify.AddUObject(this, &UKzComboInstance::OnNotify);
	Play->OnEnded.AddUObject(this, &UKzComboInstance::OnAttackEnded);
	return true;
}

bool UKzComboInstance::Chain(FGameplayTag Input)
{
	const FKzComboNode_Attack* Attack = GetAttack();
	const FKzGraphData& Data = Combo->GetData();
	const int32 Output = Attack ? Data.FindOutput(Node, Attack->GetOutputName(Input, bHit)) : INDEX_NONE;
	const TConstArrayView<FKzGraphLink> Links = Output != INDEX_NONE ? Data.GetLinks(Node, Output) : TConstArrayView<FKzGraphLink>();
	return !Links.IsEmpty() && PlayAttack(Links[0].Node);
}

void UKzComboInstance::OnNotify(const UKzAnimationNotifyState* Notify, bool bBegin)
{
	if (Notify->IsA<UKzAnimNotifyState_HitWindow>())
	{
		// Each window sweeps from where it opens, and once more as it closes, for the stretch since the last tick.
		if (bBegin)
		{
			HitWindows++;
			PreviousParents.Init(TOptional<FTransform>(), PreviousParents.Num());
			SweepHitboxes();
		}
		else
		{
			const int32 Playing = Node;
			SweepHitboxes();
			if (bRunning && Node == Playing)
			{
				HitWindows = FMath::Max(HitWindows - 1, 0);
			}
		}
		return;
	}

	if (Notify->IsA<UKzAnimNotifyState_ComboWindow>())
	{
		bComboWindow = bBegin;
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		if (bBegin && KeptInput.IsValid() && Now - KeptAt <= Combo->InputBuffer)
		{
			const FGameplayTag Input = KeptInput;
			KeptInput = FGameplayTag();
			Chain(Input);
		}
	}
}

void UKzComboInstance::OnAttackEnded(bool bInterrupted)
{
	End(bInterrupted);
}

void UKzComboInstance::SweepHitboxes()
{
	const FKzComboNode_Attack* Attack = GetAttack();
	UWorld* World = GetWorld();
	if (!Attack || !World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQuery(FCollisionObjectQueryParams::AllDynamicObjects);
	if (!Context.ObjectTypes.IsEmpty())
	{
		ObjectQuery = FCollisionObjectQueryParams(Context.ObjectTypes);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KzComboSweep), false);
	QueryParams.AddIgnoredActor(Context.User);
	if (Context.Weapon)
	{
		QueryParams.AddIgnoredActor(Context.Weapon->GetOwner());
	}

	const int32 Playing = Node;
	for (int32 Index = 0; Index < Attack->Hitboxes.Num() && PreviousParents.IsValidIndex(Index); ++Index)
	{
		const FKzComboHitbox& Hitbox = Attack->Hitboxes[Index];
		const USceneComponent* Parent = Hitbox.FindParent(Context.Mesh, Context.Weapon);
		if (!Parent || !Hitbox.Shape.IsValid())
		{
			continue;
		}

		const FTransform To = Parent->GetSocketTransform(Hitbox.Socket);
		const FTransform From = PreviousParents[Index].Get(To);
		PreviousParents[Index] = To;

		const FCollisionShape Shape = Hitbox.Shape.ToCollisionShape();
		const int32 Steps = FMath::Clamp(FMath::CeilToInt(From.GetRotation().AngularDistance(To.GetRotation()) / MaxStepAngle), 1, MaxSteps);
		FTransform Previous = Hitbox.Offset * From;
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			FTransform ParentAt;
			ParentAt.Blend(From, To, static_cast<float>(Step) / Steps);
			const FTransform Current = Hitbox.Offset * ParentAt;
			const FQuat Rotation = FQuat::Slerp(Previous.GetRotation(), Current.GetRotation(), 0.5f);

			TArray<FHitResult> Hits;
			World->SweepMultiByObjectType(Hits, Previous.GetLocation(), Current.GetLocation(), Rotation, ObjectQuery, Shape, QueryParams);

#if ENABLE_DRAW_DEBUG
			if (CVarDrawHitboxes.GetValueOnGameThread())
			{
				DrawDebugShape(World, Current.GetLocation(), Current.GetRotation(), Hitbox.Shape, Hits.IsEmpty() ? FColor::Green : FColor::Red, false, 1.0f);
			}
#endif

			for (const FHitResult& Hit : Hits)
			{
				AActor* Target = Hit.GetActor();
				if (!Target || HitActors.Contains(Target))
				{
					continue;
				}

				HitActors.Add(Target);
				bHit = true;

				FKzComboHit ComboHit;
				ComboHit.Hit = Hit;
				ComboHit.DamageMultiplier = Attack->DamageMultiplier;
				OnHit.Broadcast(ComboHit);

				// Whoever heard it may have stopped the combo or chained another attack.
				if (!bRunning || Node != Playing)
				{
					return;
				}
			}
			Previous = Current;
		}
	}
}

void UKzComboInstance::End(bool bInterrupted)
{
	if (!bRunning)
	{
		return;
	}

	bRunning = false;
	Play.Reset();
	Node = INDEX_NONE;
	HitWindows = 0;
	bComboWindow = false;
	OnEnded.Broadcast(bInterrupted);
}

const FKzComboNode_Attack* UKzComboInstance::GetAttack() const
{
	return Combo && Node != INDEX_NONE ? Combo->GetData().GetNode<FKzComboNode_Attack>(Node) : nullptr;
}