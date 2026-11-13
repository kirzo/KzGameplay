// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_MoveAroundTarget.h"
#include "StateTreeExecutionContext.h"
#include "Components/CapsuleComponent.h"
#include "Engagement/KzEngagementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"
#include "NavigationSystem.h"

namespace
{
	/** Slower than this, the pawn is stuck. */
	constexpr float StuckSpeed = 30.0f;

	/** How long it may be stuck before the move ends. */
	constexpr float StuckDuration = 0.3f;

	/** How far ahead of the pawn the navmesh's edge ends the move. */
	constexpr double EdgeLookAhead = 60.0;

	/** How far off its distance a pawn moving around the target steers back at full. */
	constexpr double RadiusCorrection = 100.0;

	/** Lifts the sweep off the floor the pawn stands on. */
	constexpr float FloorClearance = 10.0f;

	constexpr int32 ArcSegments = 4;

	FVector GetOutward(const FVector& Center, const FVector& From)
	{
		return FVector(From.X - Center.X, From.Y - Center.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	}

	/** Where a pawn at From is after moving Distance in Direction relative to Center. */
	FVector GetLocationAfter(const FVector& Center, const FVector& From, EKzMoveAroundDirection Direction, double Distance)
	{
		const FVector Outward = GetOutward(Center, From);
		switch (Direction)
		{
		case EKzMoveAroundDirection::Forward:
			return From - Outward * Distance;
		case EKzMoveAroundDirection::Backward:
			return From + Outward * Distance;
		default:
		{
			const FVector Offset(From.X - Center.X, From.Y - Center.Y, 0.0);
			const double Angle = Distance / FMath::Max(Offset.Size(), 1.0) * (Direction == EKzMoveAroundDirection::Clockwise ? 1.0 : -1.0);
			const FVector Turned = Offset.RotateAngleAxisRad(Angle, FVector::UpVector);
			return FVector(Center.X + Turned.X, Center.Y + Turned.Y, From.Z);
		}
		}
	}

	/** Whether another pawn, the level or the navmesh's edge is in the way from Start to End. */
	bool IsBlocked(const APawn& Pawn, const AActor* Target, const FVector& Start, const FVector& End)
	{
		UWorld* World = Pawn.GetWorld();
		if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Pawn.GetRootComponent()))
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(KzMoveAroundTarget), false);
			FCollisionResponseParams Responses;
			Capsule->InitSweepCollisionParams(Params, Responses);
			Params.AddIgnoredActor(&Pawn);
			Params.AddIgnoredActor(Target);

			// A little thinner and off the floor, so the ground and those it already brushes don't count
			const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius() * 0.9f, Capsule->GetScaledCapsuleHalfHeight() - FloorClearance);
			const FVector Lift(0.0, 0.0, FloorClearance);
			if (World->SweepTestByChannel(Start + Lift, End + Lift, Capsule->GetComponentQuat(), Capsule->GetCollisionObjectType(), Shape, Params, Responses))
			{
				return true;
			}
		}

		const FVector Feet = Pawn.GetNavAgentLocation() - Pawn.GetActorLocation();
		FVector HitLocation;
		return UNavigationSystemV1::NavigationRaycast(World, Start + Feet, End + Feet, HitLocation, nullptr, Pawn.GetController());
	}

	/** Which way to push the stick now: around the target keeping Radius, or straight towards or away from it. */
	FVector GetMoveInput(const FVector& Center, const FVector& From, EKzMoveAroundDirection Direction, double Radius)
	{
		const FVector Outward = GetOutward(Center, From);
		switch (Direction)
		{
		case EKzMoveAroundDirection::Forward:
			return -Outward;
		case EKzMoveAroundDirection::Backward:
			return Outward;
		default:
		{
			const FVector Around = FVector::CrossProduct(FVector::UpVector, Outward) * (Direction == EKzMoveAroundDirection::Clockwise ? 1.0 : -1.0);
			const double Correction = FMath::Clamp((Radius - FVector::Dist2D(Center, From)) / RadiusCorrection, -1.0, 1.0);
			return (Around + Outward * Correction).GetSafeNormal();
		}
		}
	}
}

//----------------------------------------------------------------------//
//  FKzStateTreeTask_ChooseMoveAroundTarget
//----------------------------------------------------------------------//

EStateTreeRunStatus FKzStateTreeTask_ChooseMoveAroundTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const APawn* Pawn = Cast<APawn>(InstanceData.Actor);
	const AActor* Target = InstanceData.Target;
	if (!Pawn || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector Center = Target->GetActorLocation();
	const FVector From = Pawn->GetActorLocation();
	const double Radius = FVector::Dist2D(Center, From);
	const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Pawn->GetRootComponent());
	const double AgentRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 0.0;

	// How far it can go towards and away from the target and stay in its area: without an engagement, short of bumping into the target
	double MaxForward = Radius - AgentRadius * 2.0;
	double MaxBackward = TNumericLimits<double>::Max();
	if (const UKzEngagementComponent* Engagement = Target->FindComponentByClass<UKzEngagementComponent>(); Engagement && !Engagement->Settings.Areas.IsEmpty())
	{
		const FKzEngagementSettings& Settings = Engagement->Settings;
		const int32 Area = Engagement->GetAreaAt(From);
		if (Settings.Areas.IsValidIndex(Area))
		{
			MaxForward = Radius - Settings.GetInnerRadius(Area) - AgentRadius;
			MaxBackward = Settings.GetOuterRadius(Area) - Radius - AgentRadius;
		}
		else
		{
			MaxForward = Area == INDEX_NONE ? 0.0 : Radius - Settings.GetOuterRadius(Settings.Areas.Num() - 1) - AgentRadius;
		}
	}

	struct FOption
	{
		EKzMoveAroundDirection Direction;
		float Weight;
		double MaxDistance;
	};
	const FOption Options[] = {
		{ EKzMoveAroundDirection::Clockwise, InstanceData.SidewaysWeight * 0.5f, InstanceData.MaxDistanceSideways },
		{ EKzMoveAroundDirection::Counterclockwise, InstanceData.SidewaysWeight * 0.5f, InstanceData.MaxDistanceSideways },
		{ EKzMoveAroundDirection::Forward, InstanceData.ForwardWeight, FMath::Min<double>(InstanceData.MaxDistanceFrontal, MaxForward) },
		{ EKzMoveAroundDirection::Backward, InstanceData.BackwardWeight, FMath::Min<double>(InstanceData.MaxDistanceFrontal, MaxBackward) },
	};

	TArray<FOption, TInlineAllocator<4>> Valid;
	float TotalWeight = 0.0f;
	for (const FOption& Option : Options)
	{
		if (Option.Weight <= 0.0f || Option.MaxDistance < InstanceData.MinDistance)
		{
			continue;
		}

		// Around the target, along the arc in a few straight steps
		const int32 Steps = Option.Direction == EKzMoveAroundDirection::Clockwise || Option.Direction == EKzMoveAroundDirection::Counterclockwise ? ArcSegments : 1;
		bool bBlocked = false;
		FVector Start = From;
		for (int32 Step = 1; Step <= Steps && !bBlocked; ++Step)
		{
			const FVector End = GetLocationAfter(Center, From, Option.Direction, InstanceData.MinDistance * Step / Steps);
			bBlocked = IsBlocked(*Pawn, Target, Start, End);
			Start = End;
		}

		if (!bBlocked)
		{
			Valid.Add(Option);
			TotalWeight += Option.Weight;
		}
	}

	if (Valid.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	float Pick = FMath::FRand() * TotalWeight;
	const FOption* Chosen = &Valid.Last();
	for (const FOption& Option : Valid)
	{
		Pick -= Option.Weight;
		if (Pick < 0.0f)
		{
			Chosen = &Option;
			break;
		}
	}

	InstanceData.Direction = Chosen->Direction;
	InstanceData.Distance = FMath::FRandRange(InstanceData.MinDistance, static_cast<float>(Chosen->MaxDistance));
	return EStateTreeRunStatus::Running;
}

//----------------------------------------------------------------------//
//  FKzStateTreeTask_MoveAroundTarget
//----------------------------------------------------------------------//

EStateTreeRunStatus FKzStateTreeTask_MoveAroundTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	APawn* Pawn = Cast<APawn>(InstanceData.Actor);
	UKzInputHandlerComponent* InputHandler = Pawn ? Pawn->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	if (!InputHandler || !InstanceData.Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector Center = InstanceData.Target->GetActorLocation();
	const FVector From = Pawn->GetActorLocation();
	InstanceData.Radius = FVector::Dist2D(Center, From);
	InstanceData.Travelled = 0.0;
	InstanceData.LastLocation = From;
	InstanceData.StuckTime = 0.0f;
	InstanceData.InputHandler = InputHandler;

	InstanceData.Engagement = InstanceData.Target->FindComponentByClass<UKzEngagementComponent>();
	if (UKzEngagementComponent* Engagement = InstanceData.Engagement.Get())
	{
		Engagement->ClaimPlace(Pawn, GetLocationAfter(Center, From, InstanceData.Direction, InstanceData.Distance));
	}

	InputHandler->SetInputAxis(KzTags::Input::Move, GetMoveInput(Center, From, InstanceData.Direction, InstanceData.Radius));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FKzStateTreeTask_MoveAroundTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const APawn* Pawn = Cast<APawn>(InstanceData.Actor);
	UKzInputHandlerComponent* InputHandler = InstanceData.InputHandler.Get();
	if (!Pawn || !InputHandler || !InstanceData.Target)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FVector Location = Pawn->GetActorLocation();
	InstanceData.Travelled += FVector::Dist2D(Location, InstanceData.LastLocation);
	InstanceData.LastLocation = Location;
	if (InstanceData.Travelled >= InstanceData.Distance)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const UKzEngagementComponent* Engagement = InstanceData.Engagement.Get();
	if (Engagement && Engagement->IsInForbiddenArea(Pawn))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.StuckTime = Pawn->GetVelocity().Size2D() < StuckSpeed ? InstanceData.StuckTime + DeltaTime : 0.0f;
	if (InstanceData.StuckTime >= StuckDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FVector Input = GetMoveInput(InstanceData.Target->GetActorLocation(), Location, InstanceData.Direction, InstanceData.Radius);
	const FVector Feet = Pawn->GetNavAgentLocation();
	FVector HitLocation;
	if (UNavigationSystemV1::NavigationRaycast(Pawn->GetWorld(), Feet, Feet + Input * EdgeLookAhead, HitLocation, nullptr, Pawn->GetController()))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	InputHandler->SetInputAxis(KzTags::Input::Move, Input);
	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_MoveAroundTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UKzInputHandlerComponent* InputHandler = InstanceData.InputHandler.Get())
	{
		InputHandler->SetInputAxis(KzTags::Input::Move, FVector::ZeroVector);
	}
	if (UKzEngagementComponent* Engagement = InstanceData.Engagement.Get())
	{
		Engagement->ReleasePlace(InstanceData.Actor);
	}
	InstanceData.InputHandler.Reset();
	InstanceData.Engagement.Reset();
}