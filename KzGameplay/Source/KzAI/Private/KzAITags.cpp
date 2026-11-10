// Copyright 2026 kirzo

#include "KzAITags.h"

namespace KzTags::AI
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sense_Begin, "AI.Sense.Begin", "StateTree event: something entered the pawn's Sensor.Perception sensor. Payload: FKzOverlapResult.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sense_End, "AI.Sense.End", "StateTree event: something left the pawn's Sensor.Perception sensor. Payload: FKzOverlapResult.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Target_Changed, "AI.Target.Changed", "StateTree event: Find Target picked another target, or lost it.");
}
