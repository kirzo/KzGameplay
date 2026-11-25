// Copyright 2026 kirzo

#include "Sensors/KzSensorTags.h"

namespace KzTags::Sensor
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Targeting, "Sensor.Targeting", "The sensor combos pick their targets with: its filters, and its shape unless the attack overrides it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perception, "Sensor.Perception", "What an AI perceives: its StateTree gets it as SensorComponent and hears what enters and leaves it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spawn, "Sensor.Spawn", "What a Spawn Component with Wait For Sensor waits for before spawning its Auto Spawn Class.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Encounter, "Sensor.Encounter", "What an encounter with Start On Sensor waits for before it starts.");
}
