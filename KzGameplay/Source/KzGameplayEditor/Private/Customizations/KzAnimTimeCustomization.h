// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class UAnimSequenceBase;

/**
 * An animation time in one row: the value, its unit, and what it comes to in the other unit at the frame rate of the animation being edited.
 * Switching unit converts the value, so the time stays the same.
 */
class FKzAnimTimeCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FKzAnimTimeCustomization>(); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils) override {}

private:
	/** Frames a second of the animation being edited; unset when it is not one. */
	TOptional<double> GetFramesPerSecond() const;

	FText GetOtherUnitText() const;
	void OnUnitChanged();

	TSharedPtr<IPropertyHandle> ValueHandle;
	TSharedPtr<IPropertyHandle> UnitHandle;
	TWeakObjectPtr<const UAnimSequenceBase> Animation;
};