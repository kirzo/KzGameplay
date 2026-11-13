// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_Attack.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "Components/StateTreeComponent.h"
#include "EngineUtils.h"
#include "KzAITags.h"

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GrantAttackCommand(
		TEXT("kz.AI.GrantAttack"),
		TEXT("Sends AI.Attack.Granted without an attack, so each tree makes its default one: to an AI at random, or to every AI with 'all'. Until there are attack tickets."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			TArray<UStateTreeComponent*> Trees;
			for (TActorIterator<AAIController> It(World); It; ++It)
			{
				if (UStateTreeComponent* Tree = It->FindComponentByClass<UStateTreeComponent>())
				{
					Trees.Add(Tree);
				}
			}

			if (!Args.IsEmpty() && Args[0] == TEXT("all"))
			{
				for (UStateTreeComponent* Tree : Trees)
				{
					Tree->SendStateTreeEvent(KzTags::AI::Attack_Granted);
				}
			}
			else if (!Trees.IsEmpty())
			{
				Trees[FMath::RandHelper(Trees.Num())]->SendStateTreeEvent(KzTags::AI::Attack_Granted);
			}
		}));
}
#endif

//----------------------------------------------------------------------//
//  FKzStateTreeTask_ReceiveAttack
//----------------------------------------------------------------------//

EStateTreeRunStatus FKzStateTreeTask_ReceiveAttack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FKzAttackRequest* Request = InstanceData.Request.GetMutablePtr(Context);
	if (!Request)
	{
		return EStateTreeRunStatus::Running;
	}

	Context.ForEachEvent([&InstanceData, Request](const FStateTreeSharedEvent& SharedEvent)
	{
		const FStateTreeEvent* Event = SharedEvent.Get();
		if (Event && Event->Tag.MatchesTagExact(KzTags::AI::Attack_Granted))
		{
			const FKzAttackRequest* Granted = Event->Payload.GetPtr<FKzAttackRequest>();
			*Request = Granted ? *Granted : InstanceData.Default;
			Request->bPending = true;
		}
		return EStateTreeLoopEvents::Next;
	});
	return EStateTreeRunStatus::Running;
}

//----------------------------------------------------------------------//
//  FKzStateTreeTask_TakeAttack
//----------------------------------------------------------------------//

EStateTreeRunStatus FKzStateTreeTask_TakeAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FKzAttackRequest* Request = InstanceData.Request.GetMutablePtr(Context);
	if (!Request || !Request->bPending)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.Inputs = Request->Inputs;
	InstanceData.NextWhen = Request->NextWhen;
	InstanceData.bLoop = Request->bLoop;
	InstanceData.Distance = Request->Distance;
	Request->bPending = false;
	return EStateTreeRunStatus::Running;
}