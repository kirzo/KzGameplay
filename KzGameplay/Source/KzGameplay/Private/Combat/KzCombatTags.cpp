// Copyright 2026 kirzo

#include "Combat/KzCombatTags.h"

namespace KzTags::Combat
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Combo hits go through, unless they are Attack.Undodgeable.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Parrying, "State.Parrying", "Combo hits within an equipped Defense item's parry angle are parried: the attacker reacts with Reaction.Parried.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking, "State.Blocking", "Combo hits within an equipped Defense item's block angle are blocked, unless they are Attack.Unblockable.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HyperArmor, "State.HyperArmor", "Takes damage without reacting.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Dead, until whoever handles death says otherwise.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Unblockable, "Attack.Unblockable", "Neither blocked nor parried.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Undodgeable, "Attack.Undodgeable", "Hits through State.Invulnerable.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_GuardBreak, "Attack.GuardBreak", "Breaks a block: Reaction.GuardBreak instead of Reaction.Block.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction, "Reaction", "Events of how someone takes a combo hit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Hit, "Reaction.Hit", "A hit whose Hit Window asks for no reaction of its own.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Block, "Reaction.Block", "The defender blocked a hit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_GuardBreak, "Reaction.GuardBreak", "A Guard Break attack broke the defender's block.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Parried, "Reaction.Parried", "The attacker's hit was parried.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Blocked, "Reaction.Blocked", "The attacker's hit was blocked: it bounces off.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Defense_Block, "Equipment.Defense.Block", "An equipped item can block.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Defense_Parry, "Equipment.Defense.Parry", "An equipped item can parry.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Direction_Front, "Direction.Front", "The hit came from the front.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Direction_Back, "Direction.Back", "The hit came from behind.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Direction_Left, "Direction.Left", "The hit came from the left.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Direction_Right, "Direction.Right", "The hit came from the right.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "Set By Caller magnitude of a combo hit's damage effect.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit, "GameplayCue.Hit", "A hit landing, as an attack's Impact Cue: particles, sound, camera shake, rumble.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Block, "GameplayCue.Block", "A hit blocked, as a Defense item's Block Cue: sparks and a clang at the item.");
}