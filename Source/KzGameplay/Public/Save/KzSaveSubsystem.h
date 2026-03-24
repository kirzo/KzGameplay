// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Serialization/KzSerializationLibrary.h"
#include "KzSaveSubsystem.generated.h"

class UKzSaveGame;
class UKzSaveComponent;

/** Global subsystem that manages saving and loading actor states across levels. */
UCLASS()
class KZGAMEPLAY_API UKzSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Creates a new save game object or loads an existing one from disk. */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	void LoadOrCreateSaveGame(const FString& SlotName);

	/** Writes the current save game object to disk. */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	void SaveGameToDisk(const FString& SlotName);

	/** Captures the current state of a saveable actor and stores it in memory. */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	void SaveSingleActor(AActor* SaveableActor);

	/** Restores the state of a single actor from the cached save memory. */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	void RestoreSingleActor(AActor* TargetActor, UKzSaveComponent* SaveComponent);

	/** Marks an actor's ID as permanently destroyed in the save file. */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	void MarkActorAsDestroyed(const FGuid& ActorID);

	/** Saves arbitrary binary data into the save file using a unique identifier. */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	void SaveCustomData(const FGuid& DataID, UPARAM(ref) const FKzSerializedData& Data);

	/**
	 * Retrieves arbitrary binary data from the save file using a unique identifier.
	 * Returns true if the data was found, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|SaveSystem")
	bool LoadCustomData(const FGuid& DataID, FKzSerializedData& OutData);

private:
	UPROPERTY()
	UKzSaveGame* CurrentSaveData;

	/** Callback fired when a new streaming level or map is fully loaded into the world. */
	void OnLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld);
};