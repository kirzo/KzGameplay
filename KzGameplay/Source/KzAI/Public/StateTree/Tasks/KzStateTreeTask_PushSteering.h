// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "KzStateTreeTask_PushSteering.generated.h"

class UKzSteeringComponent;
class UKzSteeringBehavior;
class UKzSteeringProfile;

/** Instance data struct for the Push Steering Behavior task */
USTRUCT()
struct FKzStateTreeTask_PushBehaviorInstanceData
{
	GENERATED_BODY()

	/** The steering component to apply the behavior to. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<UKzSteeringComponent> SteeringComponent = nullptr;

	/** The tag that identifies this layer in the priority stack. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag LayerTag;

	/** The priority of this layer. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 Priority = 1;

	/** The specific behaviors to instantiate and blend. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Parameter")
	TArray<TObjectPtr<UKzSteeringBehavior>> Behaviors;
};

/** StateTree task to push an array of Steering Behaviors onto a Steering Component */
USTRUCT(meta = (DisplayName = "Push Steering Behaviors", Category = "KzAI|Steering"))
struct KZAI_API FKzStateTreeTask_PushBehavior : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_PushBehaviorInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FKzStateTreeTask_PushBehavior() = default;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////

/** Instance data struct for the Push Steering Profile task */
USTRUCT()
struct FKzStateTreeTask_PushProfileInstanceData
{
	GENERATED_BODY()

	/** The steering component to apply the profile to. Bind this to the Context or an Output from another task */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<UKzSteeringComponent> SteeringComponent = nullptr;

	/** The tag that identifies this layer in the priority stack */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag LayerTag;

	/** The priority of this layer. Higher numbers override lower numbers */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 Priority = 1;

	/** The data asset containing a collection of behaviors to blend together */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UKzSteeringProfile> Profile;
};

/** StateTree task to push a preconfigured Steering Profile onto a Steering Component */
USTRUCT(meta = (DisplayName = "Push Steering Profile", Category = "KzAI|Steering"))
struct KZAI_API FKzStateTreeTask_PushProfile : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/** Ensure we're using the correct instance data struct */
	using FInstanceDataType = FKzStateTreeTask_PushProfileInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Default constructor */
	FKzStateTreeTask_PushProfile() = default;

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	/** Provides the description string for the StateTree editor */
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};