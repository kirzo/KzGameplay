// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzGraphEditor.h"

/** Asset editor of combos: the graph editor with a viewport that plays the selected attack. */
class FKzComboEditor : public FKzGraphEditor
{
public:
	/** Opens an editor on each combo, as TKzAssetTypeActions asks for. */
	static void CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, const TArray<UObject*>& Objects);

protected:
	virtual TSharedPtr<FKzGraphPreview> CreatePreview() override;
};