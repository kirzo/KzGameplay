// Copyright 2026 kirzo

#include "Cinematics/KzCinematicSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"

void UKzCinematicSubsystem::PushCinematicMode(APlayerController* Player, UObject* Source, const FKzCinematicSettings& Settings)
{
	UKzCinematicSubsystem* Subsystem = Get(Player);
	if (!Subsystem || !Source)
	{
		return;
	}

	FPlayerMode& Mode = Subsystem->Modes.FindOrAdd(Player);
	if (TPair<TWeakObjectPtr<UObject>, FKzCinematicSettings>* Existing = Mode.Sources.FindByPredicate([Source](const auto& Entry) { return Entry.Key == Source; }))
	{
		Existing->Value = Settings;
	}
	else
	{
		Mode.Sources.Emplace(Source, Settings);
	}
	Subsystem->Refresh(Player, Mode);
}

void UKzCinematicSubsystem::RemoveCinematicMode(APlayerController* Player, UObject* Source)
{
	UKzCinematicSubsystem* Subsystem = Get(Player);
	FPlayerMode* Mode = Subsystem ? Subsystem->Modes.Find(Player) : nullptr;
	if (!Mode)
	{
		return;
	}

	Mode->Sources.RemoveAll([Source](const auto& Entry) { return Entry.Key == Source; });
	Subsystem->Refresh(Player, *Mode);
}

bool UKzCinematicSubsystem::IsInCinematicMode(const APlayerController* Player)
{
	const UKzCinematicSubsystem* Subsystem = Get(Player);
	return Subsystem && Subsystem->FindSettings(Player);
}

const FKzCinematicSettings* UKzCinematicSubsystem::FindSettings(const APlayerController* Player) const
{
	const FPlayerMode* Mode = Modes.Find(Player);
	return Mode ? &Mode->Applied : nullptr;
}

bool UKzCinematicSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UKzCinematicSubsystem* UKzCinematicSubsystem::Get(const APlayerController* Player)
{
	const UWorld* World = Player ? Player->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UKzCinematicSubsystem>() : nullptr;
}

void UKzCinematicSubsystem::Refresh(APlayerController* Player, FPlayerMode& Mode)
{
	// A source destroyed without taking its request back asks for nothing anymore.
	Mode.Sources.RemoveAll([](const auto& Entry) { return !Entry.Key.IsValid(); });

	FKzCinematicSettings Wanted;
	Wanted.bBlockInput = false;
	for (const auto& Entry : Mode.Sources)
	{
		Wanted.bHidePlayer |= Entry.Value.bHidePlayer;
		Wanted.bBlockInput |= Entry.Value.bBlockInput;
		Wanted.HiddenUILayers.AppendTags(Entry.Value.HiddenUILayers);
	}

	APawn* Pawn = Player->GetPawn();
	if (Wanted.bHidePlayer != Mode.Applied.bHidePlayer)
	{
		Wanted.bHidePlayer ? HidePlayer(Mode, Pawn) : ShowPlayer(Mode);
	}
	if (Wanted.bBlockInput != Mode.Applied.bBlockInput)
	{
		Wanted.bBlockInput ? BlockInput(Mode, Player, Pawn) : UnblockInput(Mode, Player);
	}

	const bool bChanged = Wanted.bHidePlayer != Mode.Applied.bHidePlayer || Wanted.bBlockInput != Mode.Applied.bBlockInput || Wanted.HiddenUILayers != Mode.Applied.HiddenUILayers;
	Mode.Applied = Wanted;
	if (Mode.Sources.IsEmpty())
	{
		Modes.Remove(Player);
	}
	if (bChanged)
	{
		OnCinematicModeChanged.Broadcast(Player, Wanted);
	}
}

void UKzCinematicSubsystem::HidePlayer(FPlayerMode& Mode, APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	TArray<AActor*> Actors;
	Pawn->GetAttachedActors(Actors, true, true);
	Actors.Insert(Pawn, 0);
	for (AActor* Actor : Actors)
	{
		Mode.HiddenActors.Add({ Actor, Actor->IsHidden(), Actor->GetActorEnableCollision() });
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);

		// Paused animation fires no notifies, such as breathing; paused sounds go on where they were when the player shows again.
		Actor->ForEachComponent<USkeletalMeshComponent>(false, [&Mode](USkeletalMeshComponent* Mesh)
		{
			if (!Mesh->bPauseAnims)
			{
				Mesh->bPauseAnims = true;
				Mode.PausedMeshes.Add(Mesh);
			}
		});
		Actor->ForEachComponent<UAudioComponent>(false, [&Mode](UAudioComponent* Sound)
		{
			if (Sound->IsPlaying() && Sound->GetPlayState() != EAudioComponentPlayState::Paused)
			{
				Sound->SetPaused(true);
				Mode.PausedSounds.Add(Sound);
			}
		});
	}
}

void UKzCinematicSubsystem::ShowPlayer(FPlayerMode& Mode)
{
	for (const FHiddenActor& Hidden : Mode.HiddenActors)
	{
		if (AActor* Actor = Hidden.Actor.Get())
		{
			Actor->SetActorHiddenInGame(Hidden.bWasHidden);
			Actor->SetActorEnableCollision(Hidden.bHadCollision);
		}
	}
	for (const TWeakObjectPtr<USkeletalMeshComponent>& Mesh : Mode.PausedMeshes)
	{
		if (Mesh.IsValid())
		{
			Mesh->bPauseAnims = false;
		}
	}
	for (const TWeakObjectPtr<UAudioComponent>& Sound : Mode.PausedSounds)
	{
		if (Sound.IsValid())
		{
			Sound->SetPaused(false);
		}
	}
	Mode.HiddenActors.Reset();
	Mode.PausedMeshes.Reset();
	Mode.PausedSounds.Reset();
}

void UKzCinematicSubsystem::BlockInput(FPlayerMode& Mode, APlayerController* Player, APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	Mode.BlockedPawn = Pawn;
	Mode.bPawnInputWasEnabled = Pawn->InputEnabled();
	Pawn->DisableInput(Player);
	Pawn->ConsumeMovementInputVector();
	if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
	{
		Movement->StopMovementImmediately();
	}
}

void UKzCinematicSubsystem::UnblockInput(FPlayerMode& Mode, APlayerController* Player)
{
	// Only what this took: input someone else had turned off stays off.
	APawn* Pawn = Mode.BlockedPawn.Get();
	if (Pawn && Mode.bPawnInputWasEnabled && Pawn->GetController() == Player)
	{
		Pawn->EnableInput(Player);
	}
	Mode.BlockedPawn.Reset();
}