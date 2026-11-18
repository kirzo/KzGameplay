// Copyright 2026 kirzo

#include "Customizations/KzLightingRigCustomization.h"
#include "DetailLayoutBuilder.h"

void FKzLightingRigCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory("Lighting Rig", FText::GetEmpty(), ECategoryPriority::Important);
}