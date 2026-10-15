// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"

/** Rows of an item's fragments: a fragment another one requires stays until that one goes too. */
class FKzItemFragmentRowCustomizer : public FKzPropertyStackRowCustomizer
{
public:
	virtual bool CanRemoveRows(const TArray<TSharedPtr<IPropertyHandle>>& Handles, FText& OutReason) const override;
};