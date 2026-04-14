// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UKzItemDefinition;
class IDetailsView;

class FKzItemDefinitionEditor : public FAssetEditorToolkit
{
public:
	// Static factory method required by TKzAssetTypeActions template
	static TSharedRef<FKzItemDefinitionEditor> CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit);

	// Initialize the custom editor
	void InitKzItemEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UKzItemDefinition* InItemDefinition);

	// FAssetEditorToolkit interface implementation
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	// Tab Spawners
	TSharedRef<SDockTab> SpawnTab_AssetDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_FragmentStack(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_FragmentDetails(const FSpawnTabArgs& Args);

	// The asset currently being edited
	UKzItemDefinition* ItemDefinition;

	// Slate Property Editor Widgets
	TSharedPtr<IDetailsView> AssetDetailsView;
	TSharedPtr<IDetailsView> FragmentDetailsView;

	// Tab Identifiers
	static const FName AssetDetailsTabId;
	static const FName FragmentStackTabId;
	static const FName FragmentDetailsTabId;

	// Callback to hide specific properties in the left panel
	bool IsAssetPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;

	// Callback when a fragment is selected in our custom widget
	void OnFragmentSelected(class UKzItemFragment* SelectedFragment);

	// The custom widget we just created
	TSharedPtr<class SKzFragmentStack> FragmentStackWidget;
};