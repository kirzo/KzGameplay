// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "KzStateTreeTask_MoveAroundTarget.generated.h"

class UKzEngagementComponent;
class UKzInputHandlerComponent;

/** Which way a pawn moves relative to its target. */
UENUM(BlueprintType)
enum class EKzMoveAroundDirection : uint8
{
	/** Straight towards the target. */
	Forward,

	/** Straight away from the target. */
	Backward,

	/** Around the target, clockwise as seen from above. */
	Clockwise,

	/** Around the target, counterclockwise as seen from above. */
	Counterclockwise
};

USTRUCT()
struct FKzStateTreeTask_ChooseMoveAroundTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** What to move around, usually the tree's Target parameter. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;

	/** The shortest move: a direction without this much room is left out. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "Centimeters"))
	float MinDistance = 150.0f;

	/** The longest move around the target. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxDistanceSideways = 400.0f;

	/** The longest move towards or away from the target. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxDistanceFrontal = 250.0f;

	/** How likely a move around the target is, either way. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float SidewaysWeight = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float ForwardWeight = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float BackwardWeight = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	EKzMoveAroundDirection Direction = EKzMoveAroundDirection::Clockwise;

	/** How far to move: at least Min Distance. */
	UPROPERTY(EditAnywhere, Category = "Output")
	float Distance = 0.0f;
};

/**
 * Chooses which way a pawn moves relative to its target, and how far: towards it, away from it, or around it either way. A direction is left
 * out when the first Min Distance of it is blocked, by another pawn, the level or the navmesh's edge, or when it can't go that far and stay in
 * the area of the target's engagement it is in: forwards ends where the forbidden area or the area inside begins. Of the rest, one by weight,
 * at a random distance up to its maximum. Fails when none is left. For a Move Around Target after it in the same state.
 */
USTRUCT(meta = (DisplayName = "Choose Move Around Target", Category = "KzAI|Movement"))
struct KZAI_API FKzStateTreeTask_ChooseMoveAroundTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FKzStateTreeTask_ChooseMoveAroundTarget()
	{
		bShouldCallTick = false;
	}

	using FInstanceDataType = FKzStateTreeTask_ChooseMoveAroundTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FKzStateTreeTask_MoveAroundTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** What to move around, usually the tree's Target parameter. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	EKzMoveAroundDirection Direction = EKzMoveAroundDirection::Clockwise;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (Units = "Centimeters"))
	float Distance = 0.0f;

	/** The distance to the target it keeps while moving around it. */
	double Radius = 0.0;

	double Travelled = 0.0;
	FVector LastLocation = FVector::ZeroVector;

	/** How long it has barely moved. */
	float StuckTime = 0.0f;

	TWeakObjectPtr<UKzEngagementComponent> Engagement;
	TWeakObjectPtr<UKzInputHandlerComponent> InputHandler;
};

/**
 * Moves a pawn relative to its target through its Move input, as a player would: around it in an arc that keeps its distance, or straight
 * towards or away from it, following the target as it moves. Succeeds once it has covered Distance, or when it gets stuck, reaches the
 * navmesh's edge or steps into the forbidden area of the target's engagement. Its end counts as the pawn's place in that engagement from the
 * start; stopped short, where it stands becomes its place. It doesn't turn the pawn: a focus on the target and a strafe keep it facing it.
 */
USTRUCT(meta = (DisplayName = "Move Around Target", Category = "KzAI|Movement"))
struct KZAI_API FKzStateTreeTask_MoveAroundTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_MoveAroundTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};