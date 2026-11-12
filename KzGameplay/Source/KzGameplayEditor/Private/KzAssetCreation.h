// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Factories/Factory.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Styling/SlateIconFinder.h"
#include "Textures/SlateIcon.h"

/** Creating the assets that go with another one, next to those the project already has. */
namespace KzAssetCreation
{
	inline FString FolderOf(const UObject& Asset)
	{
		return FPackageName::GetLongPackagePath(Asset.GetPackage()->GetName());
	}

	/**
	 * Folder of the first asset Pick finds: in the anchor, then in the assets of its class beside it, then in any other.
	 * With none to go by, the anchor's own folder. Pick takes a const TAsset& and returns a const UObject*, or null.
	 */
	template <typename TAsset, typename TPick>
	FString InferFolder(const TAsset& Anchor, TPick&& Pick)
	{
		if (const UObject* Found = Pick(Anchor))
		{
			return FolderOf(*Found);
		}

		const FName OwnFolder(*FolderOf(Anchor));

		TArray<FAssetData> Others;
		IAssetRegistry::GetChecked().GetAssetsByClass(TAsset::StaticClass()->GetClassPathName(), Others);
		Others.StableSort([OwnFolder](const FAssetData& A, const FAssetData& B) { return A.PackagePath == OwnFolder && B.PackagePath != OwnFolder; });

		for (const FAssetData& Other : Others)
		{
			const TAsset* OtherAsset = Cast<TAsset>(Other.GetAsset());
			if (const UObject* Found = OtherAsset ? Pick(*OtherAsset) : nullptr)
			{
				return FolderOf(*Found);
			}
		}

		return OwnFolder.ToString();
	}

	/** The asset of that name in the folder, or a new one from the factory. Asking where lets the user rename it, move it or cancel. */
	inline UObject* FindOrCreate(const FString& Name, const FString& Folder, UFactory* Factory, bool bAskWhere)
	{
		if (UObject* Existing = IAssetRegistry::GetChecked().GetAssetByObjectPath(FSoftObjectPath(Folder / Name + TEXT(".") + Name)).GetAsset())
		{
			return Existing;
		}

		IAssetTools& AssetTools = IAssetTools::Get();
		return bAskWhere
			? AssetTools.CreateAssetWithDialog(Name, Folder, Factory->GetSupportedClass(), Factory)
			: AssetTools.CreateAsset(Name, Folder, Factory->GetSupportedClass(), Factory);
	}

	/** The class icon wearing the engine's "+" badge, as the level editor's add-content button does. */
	inline FSlateIcon AddIconFor(const UClass* Class)
	{
		const FSlateIcon ClassIcon = FSlateIconFinder::FindIconForClass(Class);
		return FSlateIcon(ClassIcon.GetStyleSetName(), ClassIcon.GetStyleName(), NAME_None, "LevelEditor.OpenAddContent.Overlay");
	}
}