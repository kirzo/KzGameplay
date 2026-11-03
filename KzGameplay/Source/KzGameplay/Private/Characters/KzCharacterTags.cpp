// Copyright 2026 kirzo

#include "Characters/KzCharacterTags.h"

namespace KzTags::Character
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Character.Player", "Driven by a player.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy, "Character.Enemy", "Hostile to the player.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NPC, "Character.NPC", "Neither the player nor hostile: an ally or a bystander.");
}
