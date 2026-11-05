// Copyright 2026 kirzo

#include "Input/KzUIIconSource_InputTag.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputProfile.h"
#include "Kismet/KzSystemLibrary.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"

const UInputAction* FKzUIIconSource_InputTag::GetInputAction(const FKzUIIconContext& Context) const
{
	const APlayerController* Controller = Context.Player ? Context.Player->GetPlayerController(Context.Player->GetWorld()) : nullptr;
	const UKzInputHandlerComponent* Handler = Controller ? UKzSystemLibrary::FindComponentInActorOrController<UKzInputHandlerComponent>(Controller->GetPawn()) : nullptr;
	const FKzInputAction* Config = Handler ? Handler->FindActionConfig(InputTag) : nullptr;
	return Config ? Config->InputAction.Get() : nullptr;
}

bool FKzUIIconSource_InputTag::InitFromToken(const FString& Token, const UKzUIIconTheme* Theme)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Token), false);
	if (!Tag.IsValid())
	{
		return false;
	}
	InputTag = Tag;
	return true;
}