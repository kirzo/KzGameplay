// Copyright 2026 kirzo

#include "Save/KzSaveSubsystem.h"
#include "Save/KzSaveGame.h"
#include "Save/KzSaveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Level.h"
#include "Engine/World.h"

#include "Algo/Transform.h"

void UKzSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind to level loading events to catch actors before their BeginPlay logic fires
	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UKzSaveSubsystem::OnLevelAddedToWorld);
}

void UKzSaveSubsystem::Deinitialize()
{
	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	Super::Deinitialize();
}

void UKzSaveSubsystem::LoadOrCreateSaveGame(const FString& SlotName)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveData = Cast<UKzSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}
	else
	{
		CurrentSaveData = Cast<UKzSaveGame>(UGameplayStatics::CreateSaveGameObject(UKzSaveGame::StaticClass()));
	}
}

void UKzSaveSubsystem::SaveGameToDisk(const FString& SlotName)
{
	if (CurrentSaveData)
	{
		UGameplayStatics::SaveGameToSlot(CurrentSaveData, SlotName, 0);
	}
}

void UKzSaveSubsystem::OnLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld)
{
	if (!CurrentSaveData || !InLevel)
	{
		return;
	}

	// Iterate through all actors belonging to the newly loaded level
	for (AActor* Actor : InLevel->Actors)
	{
		if (Actor)
		{
			// Check if the actor opted-in to the save system
			if (UKzSaveComponent* SaveComp = Actor->FindComponentByClass<UKzSaveComponent>())
			{
				RestoreSingleActor(Actor, SaveComp);
			}
		}
	}
}

void UKzSaveSubsystem::RestoreSingleActor(AActor* TargetActor, UKzSaveComponent* SaveComp)
{
	if (!CurrentSaveData || !TargetActor || !SaveComp || !SaveComp->UniqueSaveID.IsValid())
	{
		return;
	}

	FGuid ActorID = SaveComp->UniqueSaveID;

	// 1. Check if it was permanently destroyed in a previous session
	if (CurrentSaveData->DestroyedActors.Contains(ActorID))
	{
		TargetActor->Destroy();
		return;
	}

	// 2. Check if we have serialized data for this actor
	if (FKzActorSaveRecord* FoundRecord = CurrentSaveData->SavedActorsState.Find(ActorID))
	{
		// Restore Main Actor properties if requested
		if (SaveComp->SaveTarget == EKzSaveTarget::ActorOnly || SaveComp->SaveTarget == EKzSaveTarget::ActorAndComponents)
		{
			UKzSerializationLibrary::DeserializeObject(TargetActor, FoundRecord->ActorData, EKzSerializationMode::SaveGameProperties);
		}

		// Restore Component properties if requested
		if (SaveComp->SaveTarget == EKzSaveTarget::SpecificComponents || SaveComp->SaveTarget == EKzSaveTarget::ActorAndComponents)
		{
			TArray<UActorComponent*> AllComponents;
			TargetActor->GetComponents(AllComponents);

			// Cache the target names for fast FName lookups if restricted
			TArray<FName> SpecificNames;
			if (SaveComp->SaveTarget == EKzSaveTarget::SpecificComponents)
			{
				Algo::Transform(SaveComp->ComponentsToSave, SpecificNames, &FKzComponentReference::ComponentName);
			}

			for (UActorComponent* Comp : AllComponents)
			{
				// Skip the save component itself to avoid cyclic logic
				if (Comp == SaveComp) continue;

				FName CompFName = Comp->GetFName();

				// For SpecificComponents mode, ensure this component's FName is in the allowed list
				if (SaveComp->SaveTarget == EKzSaveTarget::SpecificComponents && !SpecificNames.Contains(CompFName))
				{
					continue;
				}

				// The map keys are FStrings, so we convert once we know it's a valid match
				if (FKzSerializedData* CompData = FoundRecord->ComponentData.Find(CompFName.ToString()))
				{
					UKzSerializationLibrary::DeserializeObject(Comp, *CompData, EKzSerializationMode::SaveGameProperties);
				}
			}
		}
	}

	// 3. Mark as processed and notify Blueprints
	SaveComp->NotifyStateRestored();
}

void UKzSaveSubsystem::SaveSingleActor(AActor* SaveableActor)
{
	if (!CurrentSaveData || !SaveableActor)
	{
		return;
	}

	UKzSaveComponent* SaveComp = SaveableActor->FindComponentByClass<UKzSaveComponent>();
	if (!SaveComp || !SaveComp->UniqueSaveID.IsValid())
	{
		return;
	}

	FKzActorSaveRecord NewRecord;

	// Serialize Main Actor properties
	if (SaveComp->SaveTarget == EKzSaveTarget::ActorOnly || SaveComp->SaveTarget == EKzSaveTarget::ActorAndComponents)
	{
		NewRecord.ActorData = UKzSerializationLibrary::SerializeObject(SaveableActor, EKzSerializationMode::SaveGameProperties);
	}

	// Serialize Component properties
	if (SaveComp->SaveTarget == EKzSaveTarget::SpecificComponents || SaveComp->SaveTarget == EKzSaveTarget::ActorAndComponents)
	{
		TArray<UActorComponent*> AllComponents;
		SaveableActor->GetComponents(AllComponents);

		TArray<FName> SpecificNames;
		if (SaveComp->SaveTarget == EKzSaveTarget::SpecificComponents)
		{
			Algo::Transform(SaveComp->ComponentsToSave, SpecificNames, &FKzComponentReference::ComponentName);
		}

		for (UActorComponent* Comp : AllComponents)
		{
			if (Comp == SaveComp) continue;

			FName CompFName = Comp->GetFName();

			if (SaveComp->SaveTarget == EKzSaveTarget::SpecificComponents && !SpecificNames.Contains(CompFName))
			{
				continue;
			}

			FKzSerializedData CompData = UKzSerializationLibrary::SerializeObject(Comp, EKzSerializationMode::SaveGameProperties);
			NewRecord.ComponentData.Add(CompFName.ToString(), CompData);
		}
	}

	CurrentSaveData->SavedActorsState.Add(SaveComp->UniqueSaveID, NewRecord);
}

void UKzSaveSubsystem::MarkActorAsDestroyed(const FGuid& ActorID)
{
	if (CurrentSaveData && ActorID.IsValid())
	{
		CurrentSaveData->DestroyedActors.Add(ActorID);
		// Clean up memory to keep save files small
		CurrentSaveData->SavedActorsState.Remove(ActorID);
	}
}

void UKzSaveSubsystem::SaveCustomData(const FGuid& DataID, const FKzSerializedData& Data)
{
	if (CurrentSaveData && DataID.IsValid())
	{
		CurrentSaveData->CustomData.Add(DataID, Data);
	}
}

bool UKzSaveSubsystem::LoadCustomData(const FGuid& DataID, FKzSerializedData& OutData)
{
	if (CurrentSaveData && DataID.IsValid())
	{
		if (const FKzSerializedData* FoundData = CurrentSaveData->CustomData.Find(DataID))
		{
			OutData = *FoundData;
			return true;
		}
	}
	return false;
}