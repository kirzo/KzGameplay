// Copyright 2026 kirzo

#pragma once

#include "NativeGameplayTags.h"

/**
 * Tags the interaction system speaks natively, so the pieces that ship together find each other without
 * being wired up by hand. The properties carrying them stay editable for anything that wants its own channel.
 */
namespace KzTags::Interaction
{
	/** An interaction began. Carries the interactable as OptionalObject. */
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Begun);

	/** An interaction ended, whatever ended it. */
	KZGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ended);
}
