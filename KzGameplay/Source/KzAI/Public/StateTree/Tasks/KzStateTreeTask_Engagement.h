// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "KzStateTreeTask_Engagement.generated.h"

class UKzEngagementComponent;

USTRUCT()
struct FKzStateTreeTask_EngageTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Whose engagement it joins, usually the tree's Target parameter. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;

	/** How much of a sector it takes. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Weight = 5.0f;

	/** The area it goes to while it has room, by its tag. None prefers the first one. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag PreferredArea;

	TWeakObjectPtr<UKzEngagementComponent> Engagement;
};

/**
 * Joins the engagement of the target, so the pawn counts there with its weight and preferred area, and leaves it on exit. Meant as a global
 * task after Find Target: it moves to a new target's engagement on AI.Target.Changed, as it only works on events.
 */
USTRUCT(meta = (DisplayName = "Engage Target", Category = "KzAI|Engagement"))
struct KZAI_API FKzStateTreeTask_EngageTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FKzStateTreeTask_EngageTarget()
	{
		bShouldCallTick = false;
		bShouldCallTickOnlyOnEvents = true;
	}

	using FInstanceDataType = FKzStateTreeTask_EngageTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FKzStateTreeTask_FindEngagementPointInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Whose engagement it places the pawn in, usually the tree's Target parameter. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;

	/**
	 * Where to head now, for a Move To in the same state with a Destination Move Tolerance: the place, or a point ahead around the target when
	 * the straight way passes too close to it. It moves with the target.
	 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FVector Location = FVector::ZeroVector;

	TWeakObjectPtr<UKzEngagementComponent> Engagement;
};

/**
 * Sets the pawn on its way to its place in the target's engagement, picking one if it has none or the one it has is no longer right. The
 * place counts as its own from the moment it sets off, and it keeps it on exit if it got there. On the way it keeps Location up to date,
 * and may take a better place or swap with another attacker. Fails if the pawn hasn't engaged the target, such as without Engage Target.
 */
USTRUCT(meta = (DisplayName = "Find Engagement Point", Category = "KzAI|Engagement"))
struct KZAI_API FKzStateTreeTask_FindEngagementPoint : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_FindEngagementPointInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};