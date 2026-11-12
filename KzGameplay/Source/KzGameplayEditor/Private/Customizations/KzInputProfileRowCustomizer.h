// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"

class UKzInputProfile;
struct FGameplayTag;

/**
 * What the profile editor's toolbar does, for other tools to reuse. Asking where shows the save dialog; otherwise the
 * asset lands next to the ones the project already has.
 */
namespace KzInputProfileTools
{
	/** Adds an entry for the tag with the Input Action named after it, found or created, and maps it in the profile's context with its key left to assign. */
	void AddActionFromTag(UKzInputProfile& Profile, const FGameplayTag& Tag, bool bAskWhere);

	/** Gives the profile the Mapping Context named after it, found or created, with its actions mapped and their keys left to assign. */
	void CreateMappingContext(UKzInputProfile& Profile, bool bAskWhere);
}

/** Toolbar tools of the input profile editor: add an action from its tag and create the mapping context, both next to the ones the project already has. */
class FKzInputProfileRowCustomizer : public FKzPropertyStackRowCustomizer
{
public:
	virtual void OnRegister(FAssetEditorToolkit* InHostEditor) override;
};