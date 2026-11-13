// Copyright 2026 kirzo

#pragma once

#include "NativeGameplayTags.h"

namespace KzTags::Character
{
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player);

	/** The character can jump: grants UKzGameplayAbility_Jump. */
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Capability_Jump);
}
