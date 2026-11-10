// Copyright 2026 kirzo

#include "Sensors/KzSensorTags.h"

namespace KzTags::Sensor
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Targeting, "Sensor.Targeting", "The sensor combos pick their targets with: its filters, and its shape unless the attack overrides it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perception, "Sensor.Perception", "What an AI perceives: its StateTree gets it as SensorComponent and hears what enters and leaves it.");
}
