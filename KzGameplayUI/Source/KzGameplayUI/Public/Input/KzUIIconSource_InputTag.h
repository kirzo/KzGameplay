// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KzUIIconSource.h"
#include "KzUIIconSource_InputTag.generated.h"

/** The key of the action an input tag fires for the player now: the action the topmost input profile of their pawn's handler gives the tag, so an item that retunes the tag shows its own key. */
USTRUCT(meta = (DisplayName = "Input Tag"))
struct KZGAMEPLAYUI_API FKzUIIconSource_InputTag : public FKzUIIconSource_InputActionBase
{
	GENERATED_BODY()

	UPROPERTY(Category = Icon, EditAnywhere, meta = (Categories = "Input"))
	FGameplayTag InputTag;

	virtual const UInputAction* GetInputAction(const FKzUIIconContext& Context) const override;
	virtual FText GetPreviewText() const override { return FText::FromName(InputTag.GetTagName()); }

	/** Takes any gameplay tag that exists, such as {Input.Interact}. */
	virtual bool InitFromToken(const FString& Token, const UKzUIIconTheme* Theme) override;
	virtual int32 GetTokenOrder() const override { return 200; }
};