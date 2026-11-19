// Copyright 2026 kirzo

#include "StateTree/Conditions/KzStateTreeCondition_Attack.h"
#include "StateTreeExecutionContext.h"
#include "Combat/KzCombat.h"
#include "Combat/KzCombatSubsystem.h"
#include "GameFramework/Pawn.h"

bool FKzStateTreeCondition_HasAttack::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const APawn* Pawn = Cast<APawn>(InstanceData.Actor);
	const UKzCombatSubsystem* Subsystem = UKzCombatSubsystem::Get(Pawn);
	const UKzCombatant* Combatant = Subsystem ? Subsystem->FindCombatant(Pawn) : nullptr;
	return (Combatant && Combatant->HasAttackToTake()) ^ bInvert;
}