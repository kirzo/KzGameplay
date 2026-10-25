// Copyright 2026 kirzo

#include "Combat/KzComboEditor.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboPreview.h"

void FKzComboEditor::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, const TArray<UObject*>& Objects)
{
	for (UObject* Object : Objects)
	{
		if (UKzComboGraph* Combo = Cast<UKzComboGraph>(Object))
		{
			MakeShared<FKzComboEditor>()->Initialize(Mode, Host, Combo);
		}
	}
}

TSharedPtr<FKzGraphPreview> FKzComboEditor::CreatePreview()
{
	UKzComboGraph* Combo = Cast<UKzComboGraph>(GetGraphAsset());
	if (!Combo)
	{
		return nullptr;
	}
	return MakeShared<FKzComboPreview>(*Combo);
}