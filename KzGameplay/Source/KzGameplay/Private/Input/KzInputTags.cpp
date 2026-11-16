// Copyright 2026 kirzo

#include "Input/KzInputTags.h"

namespace KzTags::Input
{
	UE_DEFINE_GAMEPLAY_TAG(Move, "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Look, "Input.Look");
	UE_DEFINE_GAMEPLAY_TAG(Jump, "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Strafe, "Input.Strafe", "Held, the character strafes. Analog, it is the world direction to face.");
}