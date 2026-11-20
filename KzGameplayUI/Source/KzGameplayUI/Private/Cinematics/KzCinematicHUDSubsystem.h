// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectKey.h"
#include "KzCinematicHUDSubsystem.generated.h"

class APlayerController;
struct FKzCinematicSettings;

/** Hides on each player's KzHUD the UI layers its cinematic mode asks to hide (Hidden UI Layers), and shows them again after. */
UCLASS()
class UKzCinematicHUDSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void HandleCinematicModeChanged(APlayerController* Player, const FKzCinematicSettings& Settings);

	/** The layers hidden for each player, to show again what the mode stops asking for. */
	TMap<TObjectKey<APlayerController>, FGameplayTagContainer> HiddenLayers;
};