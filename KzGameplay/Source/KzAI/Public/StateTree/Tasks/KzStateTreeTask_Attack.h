// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "KzStateTreeTask_Attack.generated.h"

class UKzCombatant;

USTRUCT()
struct FKzStateTreeTask_TakeAttackInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	TArray<FGameplayTag> Inputs;

	UPROPERTY(EditAnywhere, Category = "Output")
	FGameplayTagQuery NextWhen;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	float Distance = 0.0f;

	TWeakObjectPtr<UKzCombatant> Combatant;
};

/**
 * Takes the attack the pawn's combat gave it, for the tasks of its state and the states under it: Distance for the approach, the rest for
 * Press Input. When the state exits, the attack is over: how it went, started or not and cut short or not, decides how it cools down.
 * Fails without an attack to take.
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
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};