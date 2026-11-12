// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
enum class EKzHitDirectionPreset : uint8;

/**
 * A hit direction as its preset in one row, with the vector under it: greyed with the preset's own, editable for Custom, hidden for Hitbox Motion.
 * Picking a preset writes its vector, so Custom starts from the last one picked.
 */
class FKzHitDirectionCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FKzHitDirectionCustomization>(); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils) override;

private:
	/** False too while the selection mixes presets. */
	bool IsPreset(EKzHitDirectionPreset Preset) const;

	void OnPresetChanged();

	TSharedPtr<IPropertyHandle> PresetHandle;
	TSharedPtr<IPropertyHandle> VectorHandle;
};