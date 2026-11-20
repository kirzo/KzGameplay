// Copyright 2026 kirzo

#include "Cinematics/KzCinematicHUDSubsystem.h"
#include "Cinematics/KzCinematicSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "KzHUD.h"

void UKzCinematicHUDSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UKzCinematicSubsystem* Cinematic = Collection.InitializeDependency<UKzCinematicSubsystem>())
	{
		Cinematic->OnCinematicModeChanged.AddUObject(this, &UKzCinematicHUDSubsystem::HandleCinematicModeChanged);
	}
}

bool UKzCinematicHUDSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UKzCinematicHUDSubsystem::HandleCinematicModeChanged(APlayerController* Player, const FKzCinematicSettings& Settings)
{
	AKzHUD* HUD = Player ? Cast<AKzHUD>(Player->GetHUD()) : nullptr;
	if (!HUD)
	{
		return;
	}

	FGameplayTagContainer& Hidden = HiddenLayers.FindOrAdd(Player);
	for (const FGameplayTag& Layer : Hidden)
	{
		if (!Settings.HiddenUILayers.HasTagExact(Layer))
		{
			HUD->RemoveLayerHidden(Layer, this);
		}
	}
	for (const FGameplayTag& Layer : Settings.HiddenUILayers)
	{
		if (!Hidden.HasTagExact(Layer))
		{
			HUD->PushLayerHidden(Layer, this);
		}
	}

	if (Settings.HiddenUILayers.IsEmpty())
	{
		HiddenLayers.Remove(Player);
	}
	else
	{
		Hidden = Settings.HiddenUILayers;
	}
}