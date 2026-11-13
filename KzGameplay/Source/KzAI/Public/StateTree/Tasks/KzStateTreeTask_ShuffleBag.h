// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StateTreePropertyRef.h"
#include "Containers/KzShuffleBag.h"
#include "KzStateTreeTask_ShuffleBag.generated.h"

USTRUCT()
struct FKzStateTreeTask_ShuffleBagDrawInstanceData
{
	GENERATED_BODY()

	/** The bag to draw from: a parameter of the tree, so its round lasts while the tree runs. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TStateTreePropertyRef<FKzShuffleBag> Bag;

	/** The entry drawn. */
	UPROPERTY(EditAnywhere, Category = "Output")
	int32 Index = INDEX_NONE;

	/** The value drawn, for a bag of gameplay tags; none for any other bag. */
	UPROPERTY(EditAnywhere, Category = "Output")
	FGameplayTag Tag;
};

/**
 * Draws the next entry of a shuffle bag as its state starts, and succeeds; fails when no entry has weight. To pick a state by chance
 * without streaks, a bag of gameplay tags, one per state, and transitions on success comparing Tag, each to its state.
 */
USTRUCT(meta = (DisplayName = "Shuffle Bag Draw", Category = "KzAI|Random"))
struct KZAI_API FKzStateTreeTask_ShuffleBagDraw : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FKzStateTreeTask_ShuffleBagDraw()
	{
		bShouldCallTick = false;
	}

	using FInstanceDataType = FKzStateTreeTask_ShuffleBagDrawInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};