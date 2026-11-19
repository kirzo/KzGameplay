// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "KzStateTreeCondition_Attack.generated.h"

USTRUCT()
struct FKzStateTreeCondition_HasAttackInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;
};

/** Whether the pawn's combat gave it an attack its tree hasn't taken yet, for the state that makes it. */
USTRUCT(meta = (DisplayName = "Has Attack", Category = "KzAI|Combat"))
struct KZAI_API FKzStateTreeCondition_HasAttack : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeCondition_HasAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};