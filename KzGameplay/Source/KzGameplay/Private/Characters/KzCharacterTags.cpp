// Copyright 2026 kirzo

#include "Characters/KzCharacterTags.h"

namespace KzTags::Character
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Character.Player", "Driven by a player. Which side it is on is its faction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Capability_Jump, "Capability.Jump", "Can jump: grants UKzGameplayAbility_Jump.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Capability_Strafe, "Capability.Strafe", "Can strafe: grants UKzGameplayAbility_Strafe.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Strafing, "State.Strafing", "Faces its control rotation instead of its movement.");
}
