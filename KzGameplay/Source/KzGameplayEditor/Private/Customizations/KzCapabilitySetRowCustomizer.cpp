// Copyright 2026 kirzo

#include "Customizations/KzCapabilitySetRowCustomizer.h"
#include "Customizations/KzInputProfileRowCustomizer.h"
#include "Factories/KzInputProfileAssetFactory.h"
#include "KzAssetCreation.h"
#include "Abilities/KzGameplayAbility.h"
#include "Capabilities/KzCapabilitySet.h"
#include "Input/KzInputProfile.h"
#include "Editors/KzArrayAssetEditor.h"

#include "Dialog/SCustomDialog.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "GameplayTagsEditorModule.h"
#include "IDetailsView.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "KzCapabilitySetRowCustomizer"

UKzNewCapabilityOptions::UKzNewCapabilityOptions()
{
	AbilityClass = UKzGameplayAbility::StaticClass();
}

namespace
{
	const UObject* PickAbility(const UKzCapabilitySet& Set)
	{
		for (const TPair<FGameplayTag, FKzCapabilityGrant>& Pair : Set.Capabilities)
		{
			// Only an ability made in a Blueprint lives in a folder
			const UClass* AbilityClass = Pair.Value.Ability.Get();
			if (const UObject* Blueprint = AbilityClass ? AbilityClass->ClassGeneratedBy.Get() : nullptr)
			{
				return Blueprint;
			}
		}
		return nullptr;
	}

	const UObject* PickProfile(const UKzCapabilitySet& Set)
	{
		for (const TPair<FGameplayTag, FKzCapabilityGrant>& Pair : Set.Capabilities)
		{
			for (const UKzInputProfile* Profile : Pair.Value.InputProfiles)
			{
				if (Profile)
				{
					return Profile;
				}
			}
		}
		return nullptr;
	}

	/** The tag, added to the project's tag list when it does not exist yet. Invalid if it could not be added. */
	FGameplayTag FindOrAddTag(const FString& TagName)
	{
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
		if (!Tag.IsValid() && IGameplayTagsEditorModule::Get().AddNewGameplayTagToINI(TagName))
		{
			Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
		}

		if (!Tag.IsValid())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("TagFailed", "Could not create the tag {0}."), FText::FromString(TagName)));
		}
		return Tag;
	}

	bool AskOptions(UKzNewCapabilityOptions& Options)
	{
		FDetailsViewArgs Args;
		Args.bAllowSearch = false;
		Args.bHideSelectionTip = true;
		Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

		const TSharedRef<IDetailsView> Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);
		Details->SetObject(&Options);

		const TSharedRef<SCustomDialog> Dialog = SNew(SCustomDialog)
			.Title(LOCTEXT("NewCapabilityTitle", "New Capability"))
			.UseScrollBox(false)
			.Content()
			[
				SNew(SBox)
					.MinDesiredWidth(420.f)
					.MinDesiredHeight(160.f)
					[
						Details
					]
			]
			.Buttons({
				SCustomDialog::FButton(LOCTEXT("Create", "Create")),
				SCustomDialog::FButton(LOCTEXT("Cancel", "Cancel"))
			});

		return Dialog->ShowModal() == 0;
	}

	/** The input side of a capability: its tag, and the profile that declares it with its action and its mapping context. */
	UKzInputProfile* FindOrCreateInput(const UKzCapabilitySet& Set, const FString& Name, const FGameplayTag& Input)
	{
		const FString Folder = KzAssetCreation::InferFolder(Set, PickProfile);
		UKzInputProfile* Profile = Cast<UKzInputProfile>(KzAssetCreation::FindOrCreate(TEXT("IP_") + Name, Folder, NewObject<UKzInputProfileAssetFactory>(), false));
		if (!Profile)
		{
			return nullptr;
		}

		if (!Profile->FindActionConfigForTag(Input))
		{
			KzInputProfileTools::AddActionFromTag(*Profile, Input, false);
		}
		if (!Profile->MappingContext)
		{
			KzInputProfileTools::CreateMappingContext(*Profile, false);
		}
		return Profile;
	}

	/** The ability Blueprint of a capability, listening to its input when it has one. Null if there is no such gameplay ability. */
	UClass* FindOrCreateAbility(const UKzCapabilitySet& Set, const FString& Name, UClass* ParentClass, const FGameplayTag& Input)
	{
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = ParentClass;

		UBlueprint* Blueprint = Cast<UBlueprint>(KzAssetCreation::FindOrCreate(TEXT("GA_") + Name, KzAssetCreation::InferFolder(Set, PickAbility), Factory, false));
		UClass* AbilityClass = Blueprint ? Blueprint->GeneratedClass.Get() : nullptr;
		if (!AbilityClass || !AbilityClass->IsChildOf(UGameplayAbility::StaticClass()))
		{
			return nullptr;
		}

		UKzGameplayAbility* Defaults = Cast<UKzGameplayAbility>(AbilityClass->GetDefaultObject());
		if (Defaults && Input.IsValid() && Defaults->GetInputPolicy(Input) == EKzAbilityInputPolicy::None)
		{
			FKzAbilityInput& Binding = Defaults->InputBindings.AddDefaulted_GetRef();
			Binding.InputTag = Input;
			Binding.Policy = EKzAbilityInputPolicy::ActivateAndListen;
			Blueprint->MarkPackageDirty();
		}
		return AbilityClass;
	}

	void NewCapability(UKzCapabilitySet& Set)
	{
		const TStrongObjectPtr<UKzNewCapabilityOptions> Options(NewObject<UKzNewCapabilityOptions>());
		if (!AskOptions(*Options))
		{
			return;
		}

		// The name ends up in asset names, which are the stricter of the two
		const FString Name = Options->Name.TrimStartAndEnd();
		FText Reason;
		if (Name.IsEmpty() || !FName::IsValidXName(Name, INVALID_OBJECTNAME_CHARACTERS, &Reason))
		{
			FMessageDialog::Open(EAppMsgType::Ok, Name.IsEmpty() ? LOCTEXT("NoName", "The capability needs a name.") : Reason);
			return;
		}

		const FGameplayTag Capability = FindOrAddTag(TEXT("Capability.") + Name);
		if (!Capability.IsValid())
		{
			return;
		}
		if (Set.Capabilities.Contains(Capability))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("AlreadyDefined", "This set already defines {0}."), FText::FromString(Capability.ToString())));
			return;
		}

		FKzCapabilityGrant Grant;
		Grant.bGrantedByDefault = Options->bGrantedByDefault;

		FGameplayTag Input;
		if (Options->bHasInput)
		{
			Input = FindOrAddTag(TEXT("Input.") + Name);
			UKzInputProfile* Profile = Input.IsValid() ? FindOrCreateInput(Set, Name, Input) : nullptr;
			if (!Profile)
			{
				return;
			}
			Grant.InputProfiles.Add(Profile);
		}

		if (UClass* ParentClass = Options->AbilityClass.Get())
		{
			Grant.Ability = FindOrCreateAbility(Set, Name, ParentClass, Input);
			if (!Grant.Ability)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("AbilityFailed", "Could not create the ability GA_{0}, or an asset of that name exists that is not a gameplay ability."), FText::FromString(Name)));
				return;
			}
		}

		const FScopedTransaction Transaction(LOCTEXT("NewCapability", "New Capability"));
		Set.Modify();
		Set.Capabilities.Add(Capability, Grant);
	}
}

void FKzCapabilitySetRowCustomizer::OnRegister(FAssetEditorToolkit* InHostEditor)
{
	// Captured per editor: this customizer is one instance shared by every open set.
	const TWeakObjectPtr<UKzCapabilitySet> Set = Cast<UKzCapabilitySet>(static_cast<FKzArrayAssetEditor*>(InHostEditor)->GetEditedAsset());

	const TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension("Asset", EExtensionHook::After, InHostEditor->GetToolkitCommands(), FToolBarExtensionDelegate::CreateLambda([Set](FToolBarBuilder& Toolbar)
		{
			Toolbar.BeginSection("Capability");
			{
				Toolbar.AddToolBarButton(
					FUIAction(FExecuteAction::CreateLambda([Set]() { if (UKzCapabilitySet* Target = Set.Get()) { NewCapability(*Target); } })),
					NAME_None,
					LOCTEXT("NewCapabilityButton", "New Capability"),
					LOCTEXT("NewCapabilityTip", "Add a capability with what it is made of: its tags, its ability Blueprint and its input action, mapping context and profile. Whatever already exists under the capability's name is used as it is."),
					KzAssetCreation::AddIconFor(UKzCapabilitySet::StaticClass()));
			}
			Toolbar.EndSection();
		}));

	InHostEditor->AddToolbarExtender(Extender);
}

#undef LOCTEXT_NAMESPACE