// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "GameplayTagContainer.h"
#include "KzStateTreeCondition_GameplayTags.generated.h"

//----------------------------------------------------------------------//
//  FKzStateTreeActorHasTagsCondition
//----------------------------------------------------------------------//

USTRUCT()
struct KZAI_API FKzStateTreeActorHasTagsConditionInstanceData
{
	GENERATED_BODY()

	/** The actor we want to check. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** The container of tags to check against the actor's tags. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer TagsToCheck;
};

/** Condition that checks if an Actor's Ability System Component has specific Gameplay Tags. */
USTRUCT(meta = (DisplayName = "Actor Has Gameplay Tags", Category = "Gameplay Tags"))
struct KZAI_API FKzStateTreeActorHasTagsCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeActorHasTagsConditionInstanceData;

	FKzStateTreeActorHasTagsCondition() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	/** Require ALL tags to match, or ANY tag? */
	UPROPERTY(EditAnywhere, Category = "Condition")
	EGameplayContainerMatchType MatchType = EGameplayContainerMatchType::Any;

	/** If true, requires exact tag matches (excluding parent tags). */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bExactMatch = false;

	/** If true, the condition passes if the match FAILS. */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};

//----------------------------------------------------------------------//
//  FKzStateTreeActorMatchTagQueryCondition
//----------------------------------------------------------------------//

USTRUCT()
struct KZAI_API FKzStateTreeActorMatchTagQueryConditionInstanceData
{
	GENERATED_BODY()

	/** The actor we want to check against the query. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> TargetActor = nullptr;
};

/** Condition that checks if an Actor's Ability System Component matches a complex Gameplay Tag Query. */
USTRUCT(meta = (DisplayName = "Actor Matches Tag Query", Category = "Gameplay Tags"))
struct KZAI_API FKzStateTreeActorMatchTagQueryCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeActorMatchTagQueryConditionInstanceData;

	FKzStateTreeActorMatchTagQueryCondition() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	/** The logic query to evaluate against the actor's current tags. */
	UPROPERTY(EditAnywhere, Category = "Condition")
	FGameplayTagQuery TagQuery;

	/** If true, the condition passes if the query FAILS. */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};