// Copyright 2026 kirzo

#include "Combat/KzComboInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzCombatLibrary.h"
#include "Combat/KzCombatNotifies.h"
#include "Combat/KzCombatTags.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"
#include "Combat/KzMeshShakeSubsystem.h"
#include "Combat/KzTimeDilationSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"
#include "Items/Fragments/KzItemFragment_Defense.h"
#include "Items/KzItemDefinition.h"
#include "KzDrawDebugHelpers.h"
#include "Kismet/KzSystemLibrary.h"
#include "Scoring/KzTargetScoringLibrary.h"
#include "Sensors/KzSensorComponent.h"
#include "Sensors/KzSensorTags.h"

namespace
{
	TAutoConsoleVariable<bool> CVarDrawHitboxes(TEXT("kz.Combo.DrawHitboxes"), false, TEXT("Draws combo hitboxes as they sweep: green, red where they hit."));
	TAutoConsoleVariable<bool> CVarDrawTargeting(TEXT("kz.Combo.DrawTargeting"), false, TEXT("Draws the shape each attack looks for targets in: yellow, red when it found one."));

	constexpr float HitStopDilation = 0.01f;
	constexpr int32 MaxBranchSteps = 16;

	FVector Flatten(const FVector& Vector)
	{
		return FVector(Vector.X, Vector.Y, 0.0);
	}

	/** The node an output leads to past any branches on the way, or INDEX_NONE. A loop of branches, which validation reports, leads nowhere. */
	int32 FollowOutput(const FKzGraphData& Data, int32 Node, FName Output, const FKzComboBranchContext& Branch)
	{
		for (int32 Step = 0; Step < MaxBranchSteps; ++Step)
		{
			const int32 Index = Data.FindOutput(Node, Output);
			const TConstArrayView<FKzGraphLink> Links = Index != INDEX_NONE ? Data.GetLinks(Node, Index) : TConstArrayView<FKzGraphLink>();
			if (Links.IsEmpty())
			{
				return INDEX_NONE;
			}

			Node = Links[0].Node;
			const FKzComboNode_Branch* BranchNode = Data.GetNode<FKzComboNode_Branch>(Node);
			if (!BranchNode)
			{
				return Node;
			}

			// An output left unlinked gives way to the branch's fallback, such as Default.
			FName Fallback;
			Output = BranchNode->PickOutput(Branch, Fallback);
			const int32 Picked = Data.FindOutput(Node, Output);
			if (!Fallback.IsNone() && (Picked == INDEX_NONE || Data.GetLinks(Node, Picked).IsEmpty()))
			{
				Output = Fallback;
			}
		}
		return INDEX_NONE;
	}
}

UKzComboInstance* UKzComboInstance::StartCombo(UKzComboGraph* Combo, const FKzComboContext& Context, FGameplayTag Input, FVector Direction)
{
	if (!Combo || !Context.Mesh)
	{
		return nullptr;
	}

	const FKzGraphData& Data = Combo->GetData();
	const int32 Entry = Data.FindEntry(Input.GetTagName());
	if (Entry == INDEX_NONE)
	{
		return nullptr;
	}

	UKzComboInstance* Instance = NewObject<UKzComboInstance>(Context.User ? static_cast<UObject*>(Context.User) : GetTransientPackage());
	Instance->Combo = Combo;
	Instance->Context = Context;
	Instance->Sensor = UKzSystemLibrary::FindComponentByGameplayTag<UKzSensorComponent>(Context.User, KzTags::Sensor::Targeting);

	const int32 First = FollowOutput(Data, Entry, Input.GetTagName(), Instance->MakeBranchContext());
	if (First == INDEX_NONE)
	{
		return nullptr;
	}

	Instance->bRunning = true;
	return Instance->PlayAttack(First, Direction) ? Instance : nullptr;
}

void UKzComboInstance::Press(FGameplayTag Input, FVector Direction)
{
	if (!bRunning)
	{
		return;
	}

	if (!ComboWindows.IsEmpty())
	{
		Chain(Input, Direction);
		return;
	}

	KeptInput = Input;
	KeptDirection = Direction;
	KeptAt = Clock;
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
	const float UserDeltaTime = DeltaTime * (Context.User ? Context.User->CustomTimeDilation : 1.0f);
	Clock += UserDeltaTime;

	UpdateFacing(UserDeltaTime);
	UpdateWarp(UserDeltaTime);
	if (HitWindows > 0)
	{
		SweepHitboxes();
	}
}

TStatId UKzComboInstance::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzComboInstance, STATGROUP_Tickables);
}

bool UKzComboInstance::PlayAttack(int32 NodeIndex, const FVector& Direction)
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
	CloseComboWindows();
	HitWindows = 0;
	bOpenedHitWindow = false;
	Reaction = FGameplayTag();
	KeptInput = FGameplayTag();
	SetWarp(nullptr, 0.0f);
	HitActors.Reset();
	PreviousParents.Init(TOptional<FTransform>(), Attack->Hitboxes.Num());
	SetCancelWindows(0);

	FacingDirection = Flatten(Direction.IsNearlyZero() && Context.User ? Context.User->GetActorForwardVector() : Direction);
	Target = FindTarget(FacingDirection);
	UpdateFacing(0.0f);

	Play->OnNotify.AddUObject(this, &UKzComboInstance::OnNotify);
	Play->OnEnded.AddUObject(this, &UKzComboInstance::OnAttackEnded);
	return true;
}

bool UKzComboInstance::Chain(FGameplayTag Input, const FVector& Direction)
{
	const int32 Next = GetAttack() ? FollowOutput(Combo->GetData(), Node, Input.GetTagName(), MakeBranchContext()) : INDEX_NONE;
	return Next != INDEX_NONE && PlayAttack(Next, Direction);
}

FKzComboBranchContext UKzComboInstance::MakeBranchContext() const
{
	FKzComboBranchContext Branch;
	const UKzAnimNotifyState_ComboWindow* Window = ComboWindows.IsEmpty() ? nullptr : ComboWindows.Last().Get();
	Branch.Window = Window ? Window->Window : FGameplayTag();
	Branch.bHit = bHit;
	if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.User))
	{
		Branch.AttackerTags = ASC->GetOwnedGameplayTags();
	}
	return Branch;
}

void UKzComboInstance::OnNotify(const UKzAnimationNotifyState* Notify, bool bBegin, float Duration)
{
	if (const UKzAnimNotifyState_HitWindow* HitWindow = Cast<UKzAnimNotifyState_HitWindow>(Notify))
	{
		// Each window hits each target once, sweeping from where it opens, and once more as it closes, for the stretch since the last tick.
		if (bBegin)
		{
			HitWindows++;
			bOpenedHitWindow = true;
			Reaction = HitWindow->Reaction;
			HitActors.Reset();
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

	if (const UKzAnimNotifyState_ComboWindow* ComboWindow = Cast<UKzAnimNotifyState_ComboWindow>(Notify))
	{
		SetComboWindow(*ComboWindow, bBegin);
		if (bBegin && KeptInput.IsValid() && Clock - KeptAt <= Combo->InputBuffer)
		{
			const FGameplayTag Input = KeptInput;
			KeptInput = FGameplayTag();
			Chain(Input, KeptDirection);
		}
		return;
	}

	if (Notify->IsA<UKzAnimNotifyState_CancelWindow>())
	{
		SetCancelWindows(FMath::Max(CancelWindows + (bBegin ? 1 : -1), 0));
		return;
	}

	if (Notify->IsA<UKzAnimNotifyState_WarpWindow>())
	{
		if (bBegin)
		{
			SetWarp(Notify, Duration);
		}
		else if (Warp == Notify)
		{
			// The end arrives before the window's last step: take it, so a leap lands where it aimed.
			if (WarpTimeLeft > 0.0f)
			{
				UpdateWarp(WarpTimeLeft);
			}
			SetWarp(nullptr, 0.0f);
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
		const int32 Steps = FKzComboHitbox::GetSweepSteps(From, To);
		FTransform Previous = Hitbox.Offset * From;
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			const FTransform Current = Hitbox.GetTransform(From, To, static_cast<float>(Step) / Steps);
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
				AActor* HitActor = Hit.GetActor();
				if (!HitActor || HitActors.Contains(HitActor))
				{
					continue;
				}

				FKzComboHit ComboHit;
				ComboHit.Hit = Hit;
				ComboHit.Result = UKzCombatLibrary::ResolveHit(Context.User, HitActor, Attack->AttackTags, ComboHit.Defense);
				ComboHit.DamageMultiplier = Attack->DamageMultiplier;
				ComboHit.Reaction = Reaction;
				ComboHit.AttackTags = Attack->AttackTags;
				ComboHit.ImpactCue = Attack->ImpactCue;

				// Ignored targets can still be hit if they stop being invulnerable while the window lasts.
				if (!ComboHit.Counts())
				{
					continue;
				}

				HitActors.Add(HitActor);
				bHit = true;
				DilateTime(ComboHit, *HitActor);
				if (ComboHit.Result == EKzHitResult::Hit && Context.User)
				{
					UKzMeshShakeSubsystem::ShakeMesh(HitActor, Flatten(HitActor->GetActorLocation() - Context.User->GetActorLocation()), Attack->HitShake, Attack->HitStop);
				}
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

AActor* UKzComboInstance::FindTarget(const FVector& Direction) const
{
	if (Context.Target)
	{
		return Context.Target;
	}

	const AActor* User = Context.User;
	UKzSensorComponent* TargetSensor = Sensor.Get();
	const FKzComboNode_Attack* Attack = GetAttack();
	FKzShapeInstance Shape;
	FTransform Transform;
	if (!User || !TargetSensor || !Attack || !GetTargetingShape(*Attack, TargetSensor, User->GetActorTransform(), Direction, Shape, Transform))
	{
		return nullptr;
	}

	TArray<FKzOverlapResult> Candidates;
	TargetSensor->Query(Shape, Transform, Candidates);

	const FVector Origin = User->GetActorLocation();
	const FKzTransformSource OriginSource(FTransform(Direction.Rotation(), Origin));

	AActor* Best = nullptr;
	double BestScore = -UE_BIG_NUMBER;
	for (const FKzOverlapResult& Overlap : Candidates)
	{
		AActor* Candidate = Overlap.Actor.Get();
		if (!Candidate || Candidate == User)
		{
			continue;
		}

		const double Score = Combo->TargetScoring.Entries.IsEmpty() ? -Flatten(Candidate->GetActorLocation() - Origin).Size() : UKzTargetScoringLibrary::EvaluateTarget(OriginSource, FKzTransformSource(Candidate), Combo->TargetScoring);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}

	if (CVarDrawTargeting.GetValueOnGameThread())
	{
		DrawDebugShape(User->GetWorld(), Transform.GetLocation(), Transform.GetRotation(), Shape, Best ? FColor::Red : FColor::Yellow, false, 1.0f);
	}
	return Best;
}

bool UKzComboInstance::GetTargetingShape(const FKzComboNode_Attack& Attack, const UKzSensorComponent* Sensor, const FTransform& UserTransform, const FVector& Direction, FKzShapeInstance& OutShape, FTransform& OutTransform)
{
	if (!Attack.bOverrideTargetShape && !Sensor)
	{
		return false;
	}

	OutShape = Attack.bOverrideTargetShape ? Attack.TargetShape : Sensor->Shape * Sensor->GetComponentScale();

	const FQuat Rotation = Flatten(Direction).ToOrientationQuat();
	const FVector Offset = Sensor ? UserTransform.InverseTransformPositionNoScale(Sensor->GetComponentLocation()) : FVector::ZeroVector;
	OutTransform = FTransform(Rotation, UserTransform.GetLocation() + Rotation.RotateVector(Offset));
	return true;
}

void UKzComboInstance::UpdateFacing(float DeltaTime)
{
	const FKzComboNode_Attack* Attack = GetAttack();
	AActor* User = Context.User;
	if (!Attack || !Attack->bFaceTarget || bOpenedHitWindow || !User)
	{
		return;
	}

	const AActor* TargetActor = Target.Get();
	const FVector Direction = TargetActor ? Flatten(TargetActor->GetActorLocation() - User->GetActorLocation()) : FacingDirection;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator Current = User->GetActorRotation();
	const float Desired = Direction.Rotation().Yaw;
	const float Yaw = Combo->TurnRate > 0.0f ? FMath::FixedTurn(Current.Yaw, Desired, Combo->TurnRate * DeltaTime) : Desired;
	User->SetActorRotation(FRotator(Current.Pitch, Yaw, Current.Roll));
}

void UKzComboInstance::UpdateWarp(float DeltaTime)
{
	const UKzAnimNotifyState_WarpWindow* Window = Cast<UKzAnimNotifyState_WarpWindow>(Warp.Get());
	AActor* User = Context.User;
	if (!Window || !User || DeltaTime <= 0.0f)
	{
		return;
	}

	// Without a target, a step forward; with one, as far as the gap allows, so one already close is not approached.
	FVector Direction = Flatten(User->GetActorForwardVector()).GetSafeNormal();
	float Allowed = Window->FallbackDistance - WarpMoved;
	if (const AActor* TargetActor = Target.Get())
	{
		const FVector ToTarget = Flatten(TargetActor->GetActorLocation() - User->GetActorLocation());
		const float Gap = ToTarget.Size() - User->GetSimpleCollisionRadius() - TargetActor->GetSimpleCollisionRadius() - Window->StopDistance;
		Direction = ToTarget.GetSafeNormal();
		Allowed = FMath::Min(Gap, Window->MaxDistance - WarpMoved);
	}

	const float TimeLeft = WarpTimeLeft;
	WarpTimeLeft -= DeltaTime;

	FVector Delta = FVector::ZeroVector;
	if (Allowed > 0.0f)
	{
		// What is left spreads over what is left of the window, so a moving target is followed.
		const float Step = Allowed * FMath::Min(1.0f, DeltaTime / FMath::Max(TimeLeft, DeltaTime));
		WarpMoved += Step;
		Delta = Direction * Step;
	}

	// Height over the window is 4·H·t·(1−t): the peak halfway, back down at the end.
	if (bWarpAirborne)
	{
		auto ArcHeight = [this, Window](float Left)
		{
			const float T = FMath::Clamp(1.0f - Left / WarpDuration, 0.0f, 1.0f);
			return 4.0f * Window->Height * T * (1.0f - T);
		};
		Delta.Z = ArcHeight(WarpTimeLeft) - ArcHeight(TimeLeft);
	}

	if (!Delta.IsNearlyZero())
	{
		UKzCombatLibrary::MoveBy(*User, Delta);
	}
}

void UKzComboInstance::SetWarp(const UKzAnimationNotifyState* Notify, float Duration)
{
	// The arc's vertical speed where it is left, the derivative of 4·H·t·(1−t), so it keeps falling instead of stopping in the air.
	float ExitSpeed = 0.0f;
	if (const UKzAnimNotifyState_WarpWindow* Open = bWarpAirborne ? Cast<UKzAnimNotifyState_WarpWindow>(Warp.Get()) : nullptr)
	{
		const float T = FMath::Clamp(1.0f - WarpTimeLeft / WarpDuration, 0.0f, 1.0f);
		ExitSpeed = 4.0f * Open->Height * (1.0f - 2.0f * T) / WarpDuration;
	}

	Warp = Notify;
	WarpDuration = Duration;
	WarpTimeLeft = Duration;
	WarpMoved = 0.0f;

	const UKzAnimNotifyState_WarpWindow* Window = Cast<UKzAnimNotifyState_WarpWindow>(Notify);
	const bool bAirborne = Window && Window->Height > 0.0f && Duration > 0.0f;
	if (bAirborne == bWarpAirborne)
	{
		return;
	}
	bWarpAirborne = bAirborne;

	// Flying, neither gravity nor the floor pulls it down mid-arc; falling afterwards lands it wherever the ground is.
	const ACharacter* Character = Cast<ACharacter>(Context.User);
	if (UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
	{
		if (bAirborne ? Movement->MovementMode != MOVE_None : Movement->MovementMode == MOVE_Flying)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(bAirborne ? MOVE_Flying : MOVE_Falling);
			Movement->Velocity.Z = ExitSpeed;
		}
	}

	// Flying, the move input would steer it through the air.
	if (UKzInputHandlerComponent* InputHandler = Context.User ? Context.User->FindComponentByClass<UKzInputHandlerComponent>() : nullptr)
	{
		if (bAirborne)
		{
			InputHandler->PushInputIgnore(KzTags::Input::Move, GetFName(), true, 0);
		}
		else
		{
			InputHandler->RemoveInputIgnore(KzTags::Input::Move, GetFName());
		}
	}
}

void UKzComboInstance::DilateTime(const FKzComboHit& Hit, AActor& HitActor)
{
	UWorld* World = GetWorld();
	UKzTimeDilationSubsystem* TimeDilation = World ? World->GetSubsystem<UKzTimeDilationSubsystem>() : nullptr;
	const FKzComboNode_Attack* Attack = GetAttack();
	if (!TimeDilation || !Attack)
	{
		return;
	}

	const UKzItemFragment_Defense* Defense = Hit.Defense.Item ? Hit.Defense.Item->FindFragmentByClass<UKzItemFragment_Defense>() : nullptr;
	float HitStop = 0.0f;
	switch (Hit.Result)
	{
	case EKzHitResult::Hit:
		HitStop = Attack->HitStop;
		break;
	case EKzHitResult::Blocked:
	case EKzHitResult::GuardBroken:
		HitStop = Defense ? Defense->BlockHitStop : 0.0f;
		break;
	case EKzHitResult::Parried:
		HitStop = Defense ? Defense->ParryHitStop : 0.0f;
		if (Defense)
		{
			TimeDilation->Dilate(Context.User, Defense->ParrySlowDilation, Defense->ParrySlowDuration);
		}
		break;
	default:
		break;
	}

	TimeDilation->Dilate(Context.User, HitStopDilation, HitStop);
	TimeDilation->Dilate(&HitActor, HitStopDilation, HitStop);
}

void UKzComboInstance::SetCancelWindows(int32 Count)
{
	const bool bWasCancelable = IsCancelable();
	CancelWindows = Count;
	if (bWasCancelable != IsCancelable())
	{
		OnCancelable.Broadcast(IsCancelable());
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
	CancelWindows = 0;
	CloseComboWindows();
	SetWarp(nullptr, 0.0f);
	OnEnded.Broadcast(bInterrupted);
}

void UKzComboInstance::SetComboWindow(const UKzAnimNotifyState_ComboWindow& Window, bool bOpen)
{
	const bool bWasOpen = !ComboWindows.IsEmpty();
	if (bOpen)
	{
		ComboWindows.Add(&Window);
	}
	else if (ComboWindows.Remove(&Window) == 0)
	{
		// Already closed by a new attack, as the previous one blends out.
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.User);
	if (!ASC)
	{
		return;
	}

	if (bWasOpen != !ComboWindows.IsEmpty())
	{
		if (bOpen)
		{
			ASC->AddLooseGameplayTag(KzTags::Combat::State_ComboWindow);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(KzTags::Combat::State_ComboWindow);
		}
	}

	if (Window.Window.IsValid())
	{
		if (bOpen)
		{
			ASC->AddLooseGameplayTag(Window.Window);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(Window.Window);
		}
	}
}

void UKzComboInstance::CloseComboWindows()
{
	if (ComboWindows.IsEmpty())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.User))
	{
		for (const TWeakObjectPtr<const UKzAnimNotifyState_ComboWindow>& Window : ComboWindows)
		{
			if (Window.IsValid() && Window->Window.IsValid())
			{
				ASC->RemoveLooseGameplayTag(Window->Window);
			}
		}
		ASC->RemoveLooseGameplayTag(KzTags::Combat::State_ComboWindow);
	}
	ComboWindows.Reset();
}

const FKzComboNode_Attack* UKzComboInstance::GetAttack() const
{
	return Combo && Node != INDEX_NONE ? Combo->GetData().GetNode<FKzComboNode_Attack>(Node) : nullptr;
}