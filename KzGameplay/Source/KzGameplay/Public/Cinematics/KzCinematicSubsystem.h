// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectKey.h"
#include "KzCinematicSubsystem.generated.h"

class APawn;
class APlayerController;
class UAudioComponent;
class USkeletalMeshComponent;

/** What cinematic mode takes away from gameplay while it lasts. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzCinematicSettings
{
	GENERATED_BODY()

	/**
	 * Hides the player's pawn and every actor attached to it, such as its equipment, pauses their animation, so its notify sounds
	 * (breathing, footsteps) stop, pauses their sounds and turns their collision off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic")
	bool bHidePlayer = false;

	/** Takes the gameplay input from the player's pawn and stops it where it stands. UI input keeps working. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic")
	bool bBlockInput = true;

	/** UI layers to hide, for a UI that reads them: KzGameplayUI hides them on a KzHUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic", meta = (Categories = "UI.Layer"))
	FGameplayTagContainer HiddenUILayers;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FKzOnCinematicModeChanged, APlayerController* /*Player*/, const FKzCinematicSettings& /*Settings*/);

/**
 * Cinematic mode, per player: what a cinematic or a conversation takes away from gameplay while it lasts. Requests come by source and
 * add up, so a conversation that hides the HUD and a sequence inside it that hides the player both hold. When the last one goes,
 * everything is back as it was, and what was already hidden or off before stays so.
 */
UCLASS()
class KZGAMEPLAY_API UKzCinematicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Puts the player in cinematic mode for Source, or changes what Source asks for. Remove it with the same Source. */
	UFUNCTION(BlueprintCallable, Category = "Cinematic", meta = (DefaultToSelf = "Source"))
	static void PushCinematicMode(APlayerController* Player, UObject* Source, const FKzCinematicSettings& Settings);

	/** Takes back what Source asked for; the mode ends with the last source. */
	UFUNCTION(BlueprintCallable, Category = "Cinematic", meta = (DefaultToSelf = "Source"))
	static void RemoveCinematicMode(APlayerController* Player, UObject* Source);

	UFUNCTION(BlueprintPure, Category = "Cinematic")
	static bool IsInCinematicMode(const APlayerController* Player);

	/** What the player's cinematic mode does now, all sources together; null when the player is not in it. */
	const FKzCinematicSettings* FindSettings(const APlayerController* Player) const;

	/** The player's cinematic mode started, changed or ended (then with nothing hidden or blocked). */
	FKzOnCinematicModeChanged OnCinematicModeChanged;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FHiddenActor
	{
		TWeakObjectPtr<AActor> Actor;
		bool bWasHidden = false;
		bool bHadCollision = false;
	};

	struct FPlayerMode
	{
		FPlayerMode() { Applied.bBlockInput = false; }

		TArray<TPair<TWeakObjectPtr<UObject>, FKzCinematicSettings>> Sources;

		/** What is applied now: nothing until the first Refresh. */
		FKzCinematicSettings Applied;

		TArray<FHiddenActor> HiddenActors;
		TArray<TWeakObjectPtr<USkeletalMeshComponent>> PausedMeshes;
		TArray<TWeakObjectPtr<UAudioComponent>> PausedSounds;

		TWeakObjectPtr<APawn> BlockedPawn;
		bool bPawnInputWasEnabled = false;
	};

	static UKzCinematicSubsystem* Get(const APlayerController* Player);

	/** Applies what the player's sources add up to, undoing what none asks for anymore. */
	void Refresh(APlayerController* Player, FPlayerMode& Mode);

	static void HidePlayer(FPlayerMode& Mode, APawn* Pawn);
	static void ShowPlayer(FPlayerMode& Mode);
	static void BlockInput(FPlayerMode& Mode, APlayerController* Player, APawn* Pawn);
	static void UnblockInput(FPlayerMode& Mode, APlayerController* Player);

	TMap<TObjectKey<APlayerController>, FPlayerMode> Modes;
};