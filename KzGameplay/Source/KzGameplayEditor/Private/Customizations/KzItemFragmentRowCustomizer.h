// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"

/** Rows of an item's fragments, each with its class icon: a fragment another one requires stays until that one goes too. */
class FKzItemFragmentRowCustomizer : public FKzPropertyStackRowCustomizer
{
public:
	virtual TSharedRef<SWidget> BuildLeadingWidget(TSharedPtr<IPropertyHandle> Handle) override;
	virtual bool CanRemoveRows(const TArray<TSharedPtr<IPropertyHandle>>& Handles, FText& OutReason) const override;
};