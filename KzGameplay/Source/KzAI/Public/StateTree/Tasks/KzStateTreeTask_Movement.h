// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Engine/EngineTypes.h"
#include "KzStateTreeTask_Movement.generated.h"

class ACharacter;

USTRUCT()
struct FKzStateTreeTask_SetMovementModeInstanceData
{
	GENERATED_BODY()

	/** The character whose movement mode will be changed */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character = nullptr;

	/** The new movement mode to apply (e.g., Walking, Flying, Swimming) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TEnumAsByte<EMovementMode> MovementMode = MOVE_Flying;
};

/** StateTree task to change the Unreal Character Movement Mode. */
USTRUCT(meta = (DisplayName = "Set Movement Mode", Category = "KzAI|Movement"))
struct KZAI_API FKzStateTreeTask_SetMovementMode : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_SetMovementModeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FKzStateTreeTask_SetMovementMode() = default;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};