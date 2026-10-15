// Copyright 2026 kirzo

#include "Save/KzSaveComponent.h"
#include "Save/KzSaveSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UKzSaveComponent::UKzSaveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UKzSaveComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// InitializeComponent can run in non-game (editor preview) worlds; only restore during actual play.
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	// Runs in the actor Initialize phase, before any actor in the level (or this actor, if spawned at runtime) begins play.
	if (!bHasBeenRestored && UniqueSaveID.IsValid())
	{
		UGameInstance* GameInstance = World->GetGameInstance();
		if (UKzSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UKzSaveSubsystem>() : nullptr)
		{
			SaveSubsystem->RestoreSingleActor(GetOwner(), this);
		}
	}
}

void UKzSaveComponent::CaptureState()
{
	if (UWorld* World = GetWorld())
	{
		UGameInstance* GameInstance = World->GetGameInstance();
		if (UKzSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UKzSaveSubsystem>() : nullptr)
		{
			SaveSubsystem->SaveSingleActor(GetOwner());
		}
	}
}

void UKzSaveComponent::NotifyStateRestored()
{
	bHasBeenRestored = true;
	OnStateRestored.Broadcast();
}

#if WITH_EDITOR
void UKzSaveComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	// Generate a new GUID only if we don't have one.
	if (!UniqueSaveID.IsValid())
	{
		UniqueSaveID = FGuid::NewGuid();
	}
}

void UKzSaveComponent::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		UniqueSaveID = FGuid::NewGuid();
	}
}
#endif