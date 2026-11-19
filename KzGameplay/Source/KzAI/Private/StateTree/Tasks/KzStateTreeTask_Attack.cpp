// Copyright 2026 kirzo

#include "StateTree/Tasks/KzStateTreeTask_Attack.h"
#include "StateTreeExecutionContext.h"
#include "Combat/KzCombat.h"
#include "Combat/KzCombatSubsystem.h"
#include "GameFramework/Pawn.h"

EStateTreeRunStatus FKzStateTreeTask_TakeAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
	{
		return EStateTreeRunStatus::Running;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const APawn* Pawn = Cast<APawn>(InstanceData.Actor);
	const UKzCombatSubsystem* Subsystem = UKzCombatSubsystem::Get(Pawn);
	UKzCombatant* Combatant = Subsystem ? Subsystem->FindCombatant(Pawn) : nullptr;

	FKzAttackDefinition Attack;
	if (!Combatant || !Combatant->TakeAttack(Attack))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.Combatant = Combatant;
	InstanceData.Inputs = Attack.Inputs;
	InstanceData.NextWhen = Attack.NextWhen;
	InstanceData.bLoop = Attack.bLoop;
	InstanceData.Distance = Attack.Distance;
	return EStateTreeRunStatus::Running;
}

void FKzStateTreeTask_TakeAttack::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType != EStateTreeStateChangeType::Changed)
	{
		return;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (UKzCombatant* Combatant = InstanceData.Combatant.Get())
	{
		Combatant->FinishAttack();
	}
	InstanceData.Combatant.Reset();
}