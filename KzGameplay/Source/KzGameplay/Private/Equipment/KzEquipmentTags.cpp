// Copyright 2026 kirzo

#include "Equipment/KzEquipmentTags.h"

namespace KzTags::Equipment
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Hand_Right, "Equipment.Slot.Hand.Right", "Held in the right hand.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Hand_Left, "Equipment.Slot.Hand.Left", "Held in the left hand.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Hand_Main, "Equipment.Slot.Hand.Main", "The weapon hand. Layouts alias it to a hand, the right one by default.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Hand_Off, "Equipment.Slot.Hand.Off", "The other hand. Layouts alias it to a hand, the left one by default.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Head, "Equipment.Slot.Head", "Worn on the head.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Chest, "Equipment.Slot.Chest", "Worn on the chest.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Legs, "Equipment.Slot.Legs", "Worn on the legs.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Feet, "Equipment.Slot.Feet", "Worn on the feet.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Back, "Equipment.Slot.Back", "Carried on the back, such as a sheathed weapon or a cape.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Waist, "Equipment.Slot.Waist", "Carried at the waist, such as a belt or a scabbard.");
}
