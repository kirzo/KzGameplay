// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Kz Factions in Project Settings: the relations as a matrix of the Faction.* tags, like the collision presets, where a click changes how
 * two factions see each other. Pairs listed more than once are flagged, and can be cleaned up keeping the one that counts.
 */
class FKzFactionSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FKzFactionSettingsCustomization>(); }

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
