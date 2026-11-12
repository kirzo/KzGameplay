// Copyright 2026 kirzo

#include "Customizations/KzHitDirectionCustomization.h"
#include "Combat/KzCombatTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"

void FKzHitDirectionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils)
{
	PresetHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzHitDirection, Preset));
	VectorHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzHitDirection, Vector));
	PresetHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzHitDirectionCustomization::OnPresetChanged));

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			PresetHandle->CreatePropertyValueWidget()
		];
}

void FKzHitDirectionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils)
{
	ChildBuilder.AddProperty(VectorHandle.ToSharedRef())
		.IsEnabled(TAttribute<bool>::CreateSPLambda(this, [this]() { return IsPreset(EKzHitDirectionPreset::Custom); }))
		.Visibility(TAttribute<EVisibility>::CreateSPLambda(this, [this]()
		{
			return IsPreset(EKzHitDirectionPreset::HitboxMotion) ? EVisibility::Collapsed : EVisibility::Visible;
		}));
}

bool FKzHitDirectionCustomization::IsPreset(EKzHitDirectionPreset Preset) const
{
	uint8 Value = 0;
	return PresetHandle.IsValid() && PresetHandle->GetValue(Value) == FPropertyAccess::Success && Value == static_cast<uint8>(Preset);
}

void FKzHitDirectionCustomization::OnPresetChanged()
{
	uint8 Value = 0;
	if (PresetHandle->GetValue(Value) != FPropertyAccess::Success)
	{
		return;
	}

	const EKzHitDirectionPreset Preset = static_cast<EKzHitDirectionPreset>(Value);
	if (Preset != EKzHitDirectionPreset::Custom && Preset != EKzHitDirectionPreset::HitboxMotion)
	{
		VectorHandle->SetValue(FKzHitDirection::GetPresetVector(Preset));
	}
}