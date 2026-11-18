// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/** The lighting rig shows its own settings right after the transform, above those of its parts. */
class FKzLightingRigCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FKzLightingRigCustomization>(); }

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};