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

void UKzSaveComponent::BeginPlay()
{
	Super::BeginPlay();

	// If the actor is spawned mid-game, it wasn't caught by the Subsystem's LevelAddedToWorld event.
	if (!bHasBeenRestored && UniqueSaveID.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UKzSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UKzSaveSubsystem>())
			{
				SaveSubsystem->RestoreSingleActor(GetOwner(), this);
			}
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