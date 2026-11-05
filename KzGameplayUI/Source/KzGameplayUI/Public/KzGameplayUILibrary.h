// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzGameplayUILibrary.generated.h"

class UKzInputIcon;

UCLASS()
class KZGAMEPLAYUI_API UKzGameplayUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Shows on Icon the key of the action InputTag fires for the icon's player now, following their input profiles. */
	UFUNCTION(Category = "KzGameplay|UI", BlueprintCallable)
	static void SetInputIconTag(UKzInputIcon* Icon, UPARAM(meta = (Categories = "Input")) FGameplayTag InputTag);
};