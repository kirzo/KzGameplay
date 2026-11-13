// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "ItemDefinitionActorFactory.generated.h"

/** Places an item definition dropped in a level as its World Actor Class. */
UCLASS()
class UItemDefinitionActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UItemDefinitionActorFactory();

	// UActorFactory interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual UClass* GetDefaultActorClass(const FAssetData& AssetData) override;
	// End of UActorFactory interface
};