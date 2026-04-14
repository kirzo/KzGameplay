// Copyright 2026 kirzo

#include "Editors/KzItemDefinitionEditor.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"

#include "Widgets/SKzObjectStack.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"

const FName FKzItemDefinitionEditor::AssetDetailsTabId(TEXT("KzItemEditor_AssetDetails"));
const FName FKzItemDefinitionEditor::FragmentStackTabId(TEXT("KzItemEditor_FragmentStack"));
const FName FKzItemDefinitionEditor::FragmentDetailsTabId(TEXT("KzItemEditor_FragmentDetails"));

TSharedRef<FKzItemDefinitionEditor> FKzItemDefinitionEditor::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit)
{
	TSharedRef<FKzItemDefinitionEditor> NewEditor(new FKzItemDefinitionEditor());
	if (ObjectsToEdit.Num() > 0)
	{
		if (UKzItemDefinition* ItemDef = Cast<UKzItemDefinition>(ObjectsToEdit[0]))
		{
			NewEditor->InitKzItemEditor(Mode, InitToolkitHost, ItemDef);
		}
	}
	return NewEditor;
}

void FKzItemDefinitionEditor::InitKzItemEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UKzItemDefinition* InItemDefinition)
{
	ItemDefinition = InItemDefinition;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;

	AssetDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	AssetDetailsView->SetObject(ItemDefinition);
	AssetDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FKzItemDefinitionEditor::IsAssetPropertyVisible));

	FragmentDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_KzItemEditor_Layout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.4f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(AssetDetailsTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(FragmentStackTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.6f)
				->AddTab(FragmentDetailsTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, FName("KzItemEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ItemDefinition);
}


void FKzItemDefinitionEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(NSLOCTEXT("WorkspaceMenu_KzItemEditor", "KzItemEditor", "Kz Item Editor"));

	InTabManager->RegisterTabSpawner(AssetDetailsTabId, FOnSpawnTab::CreateSP(this, &FKzItemDefinitionEditor::SpawnTab_AssetDetails))
		.SetDisplayName(NSLOCTEXT("KzItemEditor", "AssetDetailsTab", "Item Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FragmentStackTabId, FOnSpawnTab::CreateSP(this, &FKzItemDefinitionEditor::SpawnTab_FragmentStack))
		.SetDisplayName(NSLOCTEXT("KzItemEditor", "FragmentStackTab", "Fragments"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Outliner"));

	InTabManager->RegisterTabSpawner(FragmentDetailsTabId, FOnSpawnTab::CreateSP(this, &FKzItemDefinitionEditor::SpawnTab_FragmentDetails))
		.SetDisplayName(NSLOCTEXT("KzItemEditor", "FragmentDetailsTab", "Fragment Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Properties"));
}

void FKzItemDefinitionEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(AssetDetailsTabId);
	InTabManager->UnregisterTabSpawner(FragmentStackTabId);
	InTabManager->UnregisterTabSpawner(FragmentDetailsTabId);
}

TSharedRef<SDockTab> FKzItemDefinitionEditor::SpawnTab_AssetDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzItemEditor", "AssetDetailsTitle", "Item Details"))
		[
			AssetDetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKzItemDefinitionEditor::SpawnTab_FragmentStack(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzItemEditor", "FragmentStackTitle", "Fragments"))
		[
			SNew(SKzObjectStack<UKzItemFragment>)
				.OwnerAsset(ItemDefinition)
				.TargetArray(&ItemDefinition->Fragments)
				.bAllowDuplicates(false)
				.ItemName(INVTEXT("Fragment"))
				.OnItemSelected(this, &FKzItemDefinitionEditor::OnFragmentSelected)
		];
}

TSharedRef<SDockTab> FKzItemDefinitionEditor::SpawnTab_FragmentDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzItemEditor", "FragmentDetailsTitle", "Fragment Details"))
		[
			FragmentDetailsView.ToSharedRef()
		];
}

FName FKzItemDefinitionEditor::GetToolkitFName() const { return FName("KzItemDefinitionEditor"); }
FText FKzItemDefinitionEditor::GetBaseToolkitName() const { return NSLOCTEXT("KzItemEditor", "AppLabel", "Item Definition Editor"); }
FString FKzItemDefinitionEditor::GetWorldCentricTabPrefix() const { return TEXT("ItemDefinitionEditor"); }
FLinearColor FKzItemDefinitionEditor::GetWorldCentricTabColorScale() const { return FLinearColor::White; }

bool FKzItemDefinitionEditor::IsAssetPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	if (PropertyAndParent.Property.GetFName() == FName("Fragments"))
	{
		return false;
	}
	return true;
}

void FKzItemDefinitionEditor::OnFragmentSelected(UKzItemFragment* SelectedFragment)
{
	if (FragmentDetailsView.IsValid())
	{
		FragmentDetailsView->SetObject(SelectedFragment);
	}
}