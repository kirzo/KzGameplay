// Copyright 2026 kirzo

#include "Items/KzItemFragment.h"
#include "Items/KzItemInstance.h"

void UKzItemFragment::OnInstanceCreated(FKzItemInstance& Instance) const
{
	// Trigger the Blueprint event by default
	K2_OnInstanceCreated(Instance);
}