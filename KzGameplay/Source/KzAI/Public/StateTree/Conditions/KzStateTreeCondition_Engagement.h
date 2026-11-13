// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "KzStateTreeCondition_Engagement.generated.h"

UENUM()
enum class EKzEngagementCheck : uint8
{
	/** Too close to the target. */
	InForbiddenArea,

	/** Should move: in the forbidden area or outside every area, in an area it doesn't prefer while one it prefers has room, or making room for others. */
	OutOfPlace,

	/** Has never found nor claimed a place, such as just after engaging. */
	NoPlace,

	/** Its place is in the wrong area or sector, as for Out Of Place, wherever the pawn stands. */
	WrongPlace
};

USTRUCT()
struct FKzStateTreeCondition_EngagementInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Whose engagement it asks, usually the tree's Target parameter. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;
};

/** Asks the target's engagement about the pawn's place in it. False without a target or an engagement. */
USTRUCT(meta = (DisplayName = "Engagement", Category = "KzAI|Engagement"))
struct KZAI_API FKzStateTreeCondition_Engagement : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeCondition_EngagementInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	EKzEngagementCheck Check = EKzEngagementCheck::OutOfPlace;

	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};