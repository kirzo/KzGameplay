// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "KzStateTreeTask_Focus.generated.h"

class AAIController;

USTRUCT()
struct FKzStateTreeTask_SetFocusInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** What the controller looks at while the state is active. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;
};

/**
 * The AI's Look: turns the controller's rotation to the target while the state is active. The pawn follows it only where it
 * faces its control rotation, such as an AKzCharacter with one of its Face Control Rotation Tags.
 */
USTRUCT(meta = (DisplayName = "Set Focus", Category = "KzAI|Input"))
struct KZAI_API FKzStateTreeTask_SetFocus : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_SetFocusInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
