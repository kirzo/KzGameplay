// Copyright 2026 kirzo

#include "Customizations/KzInputProfileRowCustomizer.h"
#include "KzAssetCreation.h"
#include "Input/KzInputProfile.h"
#include "Editors/KzArrayAssetEditor.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "InputAction.h"
#include "InputEditorModule.h"
#include "InputMappingContext.h"
#include "SGameplayTagPicker.h"
#include "ScopedTransaction.h"
#include "TimerManager.h"

namespace
{
	const UObject* PickContext(const UKzInputProfile& Profile)
	{
		return Profile.MappingContext;
	}

	const UObject* PickAction(const UKzInputProfile& Profile)
	{
		const FKzInputAction* Entry = Profile.InputActions.FindByPredicate([](const FKzInputAction& Other) { return Other.InputAction != nullptr; });
		return Entry ? Entry->InputAction.Get() : nullptr;
	}

	/** Maps the action with its key left to assign, unless the context maps it already. */
	void MapUnbound(UInputMappingContext& Context, const UInputAction* Action)
	{
		if (Action && !Context.GetMappings().ContainsByPredicate([Action](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Action == Action; }))
		{
			Context.Modify();
			Context.MapKey(Action, FKey());
		}
	}

	TSharedRef<SWidget> BuildTagMenu(TWeakObjectPtr<UKzInputProfile> Profile)
	{
		return SNew(SGameplayTagPicker)
			.Filter(TEXT("Input"))
			.ShowMenuItems(true)
			.MaxHeight(350.f)
			.MultiSelect(false)
			.TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer() })
			.OnTagChanged_Lambda([Profile](const TArray<FGameplayTagContainer>& Containers)
				{
					const FGameplayTag Tag = Containers.IsEmpty() ? FGameplayTag() : Containers[0].First();
					if (!Tag.IsValid())
					{
						return;
					}

					FSlateApplication::Get().DismissAllMenus();

					// Next tick: the picker is still inside its own transaction here, and creating the action may open a modal dialog.
					GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateLambda([Profile, Tag]()
						{
							if (UKzInputProfile* Target = Profile.Get())
							{
								KzInputProfileTools::AddActionFromTag(*Target, Tag, true);
							}
						}));
				});
	}
}

void KzInputProfileTools::AddActionFromTag(UKzInputProfile& Profile, const FGameplayTag& Tag, bool bAskWhere)
{
	// Input.Dodge -> IA_Dodge
	const FString TagName = Tag.ToString();
	const FString Name = TEXT("IA_") + TagName.RightChop(TagName.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd) + 1);

	const UInputAction* Action = Cast<UInputAction>(KzAssetCreation::FindOrCreate(Name, KzAssetCreation::InferFolder(Profile, PickAction), NewObject<UInputAction_Factory>(), bAskWhere));
	if (!Action)
	{
		return;
	}

	const FScopedTransaction Transaction(INVTEXT("Add Input Action"));
	Profile.Modify();
	FKzInputAction& Entry = Profile.InputActions.AddDefaulted_GetRef();
	Entry.InputAction = Action;
	Entry.InputTag = Tag;

	if (UInputMappingContext* Context = Profile.MappingContext)
	{
		MapUnbound(*Context, Action);
	}
}

void KzInputProfileTools::CreateMappingContext(UKzInputProfile& Profile, bool bAskWhere)
{
	// IP_Dodge -> IMC_Dodge
	const FString ProfileName = Profile.GetName();
	const FString Name = TEXT("IMC_") + ProfileName.RightChop(ProfileName.Find(TEXT("_")) + 1);

	UInputMappingContext* Context = Cast<UInputMappingContext>(KzAssetCreation::FindOrCreate(Name, KzAssetCreation::InferFolder(Profile, PickContext), NewObject<UInputMappingContext_Factory>(), bAskWhere));
	if (!Context)
	{
		return;
	}

	const FScopedTransaction Transaction(INVTEXT("Create Mapping Context"));
	Profile.Modify();
	Profile.MappingContext = Context;

	for (const FKzInputAction& Entry : Profile.InputActions)
	{
		MapUnbound(*Context, Entry.InputAction);
	}
}

void FKzInputProfileRowCustomizer::OnRegister(FAssetEditorToolkit* InHostEditor)
{
	// Captured per editor: this customizer is one instance shared by every open profile.
	const TWeakObjectPtr<UKzInputProfile> Profile = Cast<UKzInputProfile>(static_cast<FKzArrayAssetEditor*>(InHostEditor)->GetEditedAsset());

	const TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension("Asset", EExtensionHook::After, InHostEditor->GetToolkitCommands(), FToolBarExtensionDelegate::CreateLambda([Profile](FToolBarBuilder& Toolbar)
		{
			Toolbar.BeginSection("Input");
			{
				Toolbar.AddComboButton(
					FUIAction(),
					FOnGetContent::CreateStatic(&BuildTagMenu, Profile),
					INVTEXT("Add Action From Tag"),
					INVTEXT("Add an entry for an input tag. Uses the Input Action named after the tag, or creates it, and maps it in the Mapping Context with its key left to assign."),
					KzAssetCreation::AddIconFor(UInputAction::StaticClass()));

				Toolbar.AddToolBarButton(
					FUIAction(
						FExecuteAction::CreateLambda([Profile]() { if (UKzInputProfile* Target = Profile.Get()) { KzInputProfileTools::CreateMappingContext(*Target, true); } }),
						FCanExecuteAction(),
						FIsActionChecked(),
						FIsActionButtonVisible::CreateLambda([Profile]() { return Profile.IsValid() && !Profile->MappingContext; })),
					NAME_None,
					INVTEXT("Create Mapping Context"),
					INVTEXT("Give this profile the Input Mapping Context named after it, creating it if there is none, with its actions mapped and their keys left to assign."),
					KzAssetCreation::AddIconFor(UInputMappingContext::StaticClass()));
			}
			Toolbar.EndSection();
		}));

	InHostEditor->AddToolbarExtender(Extender);
}