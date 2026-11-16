// Copyright 2026 kirzo

#include "Customizations/KzAnimTimeCustomization.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/KzAnimationNotifyState.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzAnimTimeCustomization"

void FKzAnimTimeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils)
{
	ValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzAnimTime, Value));
	UnitHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzAnimTime, Unit));
	UnitHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzAnimTimeCustomization::OnUnitChanged));

	// The object holding it, such as a notify, lives in the animation it times.
	TArray<UObject*> Outers;
	PropertyHandle->GetOuterObjects(Outers);
	for (const UObject* Outer : Outers)
	{
		Animation = Outer ? (Outer->IsA<UAnimSequenceBase>() ? Cast<UAnimSequenceBase>(Outer) : Outer->GetTypedOuter<UAnimSequenceBase>()) : nullptr;
		if (Animation.IsValid())
		{
			break;
		}
	}

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(220.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				ValueHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				UnitHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(this, &FKzAnimTimeCustomization::GetOtherUnitText)
			]
		];
}

TOptional<double> FKzAnimTimeCustomization::GetFramesPerSecond() const
{
	const UAnimSequenceBase* Sequence = Animation.Get();
	return Sequence && Sequence->GetSamplingFrameRate().IsValid() ? TOptional<double>(Sequence->GetSamplingFrameRate().AsDecimal()) : TOptional<double>();
}

FText FKzAnimTimeCustomization::GetOtherUnitText() const
{
	float Value = 0.0f;
	uint8 Unit = 0;
	const TOptional<double> FramesPerSecond = GetFramesPerSecond();
	if (!FramesPerSecond || ValueHandle->GetValue(Value) != FPropertyAccess::Success || UnitHandle->GetValue(Unit) != FPropertyAccess::Success)
	{
		return FText::GetEmpty();
	}

	FNumberFormattingOptions Format;
	Format.SetMaximumFractionalDigits(2);
	return static_cast<EKzAnimTimeUnit>(Unit) == EKzAnimTimeUnit::Frames
		? FText::Format(LOCTEXT("InSeconds", "= {0} s"), FText::AsNumber(Value / *FramesPerSecond, &Format))
		: FText::Format(LOCTEXT("InFrames", "= {0} f"), FText::AsNumber(Value * *FramesPerSecond, &Format));
}

void FKzAnimTimeCustomization::OnUnitChanged()
{
	float Value = 0.0f;
	uint8 Unit = 0;
	const TOptional<double> FramesPerSecond = GetFramesPerSecond();
	if (!FramesPerSecond || ValueHandle->GetValue(Value) != FPropertyAccess::Success || UnitHandle->GetValue(Unit) != FPropertyAccess::Success)
	{
		return;
	}

	// The unit just picked is the one it is now in; the value is still in the other. Frames round to a thousandth, so 2 frames there and back stay 2.
	const bool bToFrames = static_cast<EKzAnimTimeUnit>(Unit) == EKzAnimTimeUnit::Frames;
	const double Converted = bToFrames ? FMath::RoundToDouble(Value * *FramesPerSecond * 1000.0) / 1000.0 : Value / *FramesPerSecond;
	ValueHandle->SetValue(static_cast<float>(Converted));
}

#undef LOCTEXT_NAMESPACE