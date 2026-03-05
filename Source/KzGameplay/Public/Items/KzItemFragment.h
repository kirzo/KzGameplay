// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KzItemFragment.generated.h"

/**
 * Base class for modular item data fragments.
 * Fragments define specific properties or behaviors for an item definition.
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class KZGAMEPLAY_API UKzItemFragment : public UObject
{
	GENERATED_BODY()
};