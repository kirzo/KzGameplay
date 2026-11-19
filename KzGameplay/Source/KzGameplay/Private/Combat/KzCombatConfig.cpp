// Copyright 2026 kirzo

#include "Combat/KzCombatConfig.h"
#include "Combat/KzCombatTags.h"

UKzCombatConfig::UKzCombatConfig()
{
	BlockedTags.AddTag(KzTags::Combat::State_Attacking);
	BlockedTags.AddTag(KzTags::Combat::State_Reacting);
	BlockedTags.AddTag(KzTags::Combat::State_Dead);
}