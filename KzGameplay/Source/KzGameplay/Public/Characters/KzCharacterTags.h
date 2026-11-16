// Copyright 2026 kirzo

#pragma once

#include "NativeGameplayTags.h"

namespace KzTags::Character
{
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player);

	/** The character can jump: grants UKzGameplayAbility_Jump. */
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Capability_Jump);

	/** The character can strafe: grants UKzGameplayAbility_Strafe. */
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Capability_Strafe);

	/** Faces its control rotation instead of its movement. On AKzCharacter's Face Control Rotation Tags by default. */
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Strafing);
}
