// Copyright 2026 kirzo

#include "Customizations/KzHitDirectionCustomization.h"
#include "Combat/KzCombatSettings.h"
#include "Combat/KzCombatTypes.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzHitDirectionCustomization"

namespace
{
	FText GetPresetName(const FGameplayTag& Tag)
	{
		return FText::FromString(FName::NameToDisplayString(Tag.GetTagLeafName().ToString(), false));
	}

	FText FormatVector(const FVector& Vector)
	{
		return FText::FromString(FString::Printf(TEXT("(%s, %s, %s)"), *FString::SanitizeFloat(Vector.X), *FString::SanitizeFloat(Vector.Y), *FString::SanitizeFloat(Vector.Z)));
	}
}

void FKzHitDirectionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils)
{
	StructHandle = PropertyHandle;
	ModeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzHitDirection, Mode));
	PresetTagHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzHitDirection, PresetTag));
	VectorHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzHitDirection, Vector));

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(150.0f)
		[
			SNew(SComboButton)
			.OnGetMenuContent(FOnGetContent::CreateSP(this, &FKzHitDirectionCustomization::MakeMenu))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(TAttribute<FText>::CreateSP(this, &FKzHitDirectionCustomization::GetSelectedText))
			]
		];
}

void FKzHitDirectionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils)
{
	// A preset's vector as Project Settings has it, which is what plays, rather than the copy kept for Custom
	ChildBuilder.AddCustomRow(VectorHandle->GetPropertyDisplayName())
		.Visibility(TAttribute<EVisibility>::CreateSPLambda(this, [this]() { return IsMode(EKzHitDirectionMode::Preset) ? EVisibility::Visible : EVisibility::Collapsed; }))
		.NameContent()
		[
			VectorHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(TAttribute<FText>::CreateSPLambda(this, [this]()
			{
				const FKzHitDirection* Direction = GetSingle();
				const FVector* Vector = Direction ? FKzHitDirection::FindPreset(Direction->PresetTag) : nullptr;
				return Vector ? FormatVector(*Vector) : LOCTEXT("NoPreset", "Not in Project Settings: straight through the target");
			}))
		];

	ChildBuilder.AddProperty(VectorHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>::CreateSPLambda(this, [this]() { return IsMode(EKzHitDirectionMode::Custom) ? EVisibility::Visible : EVisibility::Collapsed; }));
}

const FKzHitDirection* FKzHitDirectionCustomization::GetSingle() const
{
	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	const FKzHitDirection* First = RawData.IsEmpty() ? nullptr : static_cast<const FKzHitDirection*>(RawData[0]);
	for (const void* Data : RawData)
	{
		const FKzHitDirection* Direction = static_cast<const FKzHitDirection*>(Data);
		if (!Direction || !First || Direction->Mode != First->Mode || (Direction->Mode == EKzHitDirectionMode::Preset && Direction->PresetTag != First->PresetTag))
		{
			return nullptr;
		}
	}
	return First;
}

bool FKzHitDirectionCustomization::IsMode(EKzHitDirectionMode Mode) const
{
	const FKzHitDirection* Direction = GetSingle();
	return Direction && Direction->Mode == Mode;
}

FText FKzHitDirectionCustomization::GetSelectedText() const
{
	const FKzHitDirection* Direction = GetSingle();
	if (!Direction)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}
	if (Direction->Mode != EKzHitDirectionMode::Preset)
	{
		return UEnum::GetDisplayValueAsText(Direction->Mode);
	}
	return FKzHitDirection::FindPreset(Direction->PresetTag) ? GetPresetName(Direction->PresetTag) : FText::Format(LOCTEXT("Missing", "{0} (missing)"), GetPresetName(Direction->PresetTag));
}

TSharedRef<SWidget> FKzHitDirectionCustomization::MakeMenu()
{
	FMenuBuilder Menu(true, nullptr);

	TArray<FGameplayTag> Tags;
	UKzCombatSettings::Get()->HitDirections.GetKeys(Tags);
	Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });

	Menu.BeginSection(NAME_None, LOCTEXT("Presets", "Project Settings"));
	for (const FGameplayTag& Tag : Tags)
	{
		Menu.AddMenuEntry(GetPresetName(Tag), FText::Format(LOCTEXT("PresetTooltip", "{0} {1}"), FText::FromName(Tag.GetTagName()), FormatVector(*FKzHitDirection::FindPreset(Tag))),
			FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &FKzHitDirectionCustomization::Select, EKzHitDirectionMode::Preset, Tag)));
	}
	Menu.EndSection();

	Menu.BeginSection(NAME_None, LOCTEXT("Other", "Other"));
	for (const EKzHitDirectionMode Mode : { EKzHitDirectionMode::HitboxMotion, EKzHitDirectionMode::Custom })
	{
		const UEnum* Enum = StaticEnum<EKzHitDirectionMode>();
		Menu.AddMenuEntry(Enum->GetDisplayNameTextByValue(static_cast<int64>(Mode)), Enum->GetToolTipTextByIndex(Enum->GetIndexByValue(static_cast<int64>(Mode))),
			FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &FKzHitDirectionCustomization::Select, Mode, FGameplayTag())));
	}
	Menu.EndSection();

	return Menu.MakeWidget();
}

void FKzHitDirectionCustomization::Select(EKzHitDirectionMode Mode, FGameplayTag Tag)
{
	const FScopedTransaction Transaction(LOCTEXT("SetHitDirection", "Set Hit Direction"));
	ModeHandle->SetValue(static_cast<uint8>(Mode));
	if (Mode == EKzHitDirectionMode::Preset)
	{
		PresetTagHandle->SetValueFromFormattedString(FString::Printf(TEXT("(TagName=\"%s\")"), *Tag.ToString()));
		if (const FVector* Vector = FKzHitDirection::FindPreset(Tag))
		{
			VectorHandle->SetValue(*Vector);
		}
	}
}

#undef LOCTEXT_NAMESPACE