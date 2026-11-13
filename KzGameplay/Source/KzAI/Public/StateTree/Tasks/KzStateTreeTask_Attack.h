// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StateTreePropertyRef.h"
#include "KzStateTreeTask_Attack.generated.h"

/** An attack the pawn may make: the inputs that make it, as Press Input presses them, and how close to the target it is made from. */
USTRUCT(BlueprintType)
struct KZAI_API FKzAttackRequest
{
	GENERATED_BODY()

	/** Whether it is waiting to be made. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	bool bPending = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (Categories = "Input"))
	TArray<FGameplayTag> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FGameplayTagQuery NextWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	bool bLoop = false;

	/** From the target's position; it may lie inside the forbidden area of its engagement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0", Units = "Centimeters"))
	float Distance = 100.0f;
};

USTRUCT()
struct FKzStateTreeTask_ReceiveAttackInstanceData
{
	GENERATED_BODY()

	/** Where the attack waits: a parameter of the tree, for the attack's state to take it. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TStateTreePropertyRef<FKzAttackRequest> Request;

	/** The attack made when AI.Attack.Granted comes without one of its own. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FKzAttackRequest Default;
};

/**
 * Keeps the attacks the pawn is granted with AI.Attack.Granted until its tree takes them, so a grant that comes while the pawn is busy,
 * such as taunting, is not lost: the state it is in can go on to attack once it is done. A global task; it only works on events.
 */
USTRUCT(meta = (DisplayName = "Receive Attack", Category = "KzAI|Combat"))
struct KZAI_API FKzStateTreeTask_ReceiveAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FKzStateTreeTask_ReceiveAttack()
	{
		bShouldCallTick = false;
		bShouldCallTickOnlyOnEvents = true;
	}

	using FInstanceDataType = FKzStateTreeTask_ReceiveAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FKzStateTreeTask_TakeAttackInstanceData
{
	GENERATED_BODY()

	/** The same parameter Receive Attack keeps it in. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TStateTreePropertyRef<FKzAttackRequest> Request;

	UPROPERTY(EditAnywhere, Category = "Output")
	TArray<FGameplayTag> Inputs;

	UPROPERTY(EditAnywhere, Category = "Output")
	FGameplayTagQuery NextWhen;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	float Distance = 0.0f;
};

/**
 * Takes the attack waiting for the pawn, for the tasks of its state and the states under it: Distance for the approach, the rest for Press
 * Input. It is no longer waiting from then on, made or not. Fails without one.
 */
USTRUCT(meta = (DisplayName = "Take Attack", Category = "KzAI|Combat"))
struct KZAI_API FKzStateTreeTask_TakeAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FKzStateTreeTask_TakeAttack()
	{
		bShouldCallTick = false;
	}

	using FInstanceDataType = FKzStateTreeTask_TakeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};