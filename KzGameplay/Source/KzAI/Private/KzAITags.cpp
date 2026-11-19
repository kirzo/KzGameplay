// Copyright 2026 kirzo

#include "KzAITags.h"

namespace KzTags::AI
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sense_Begin, "AI.Sense.Begin", "StateTree event: something entered the pawn's Sensor.Perception sensor. Payload: FKzOverlapResult.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sense_End, "AI.Sense.End", "StateTree event: something left the pawn's Sensor.Perception sensor. Payload: FKzOverlapResult.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Target_Changed, "AI.Target.Changed", "StateTree event: Find Target picked another target, or lost it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Granted, "AI.Attack.Granted", "StateTree event: its combat gave the pawn an attack to make; Take Attack takes it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_AttackGranted, "State.AttackGranted", "Has an attack from its combat, from when it is given until it is over.");
}
