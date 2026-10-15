// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/KzSerializationLibrary.h"
#include "KzSaveGame.generated.h"

/** Container holding the serialized state of a single actor and its components. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzActorSaveRecord
{
	GENERATED_BODY()

public:
	/** Binary data for the main Actor's properties. */
	UPROPERTY()
	FKzSerializedData ActorData;

	/** Binary data for the Actor's components, mapped by their component name. */
	UPROPERTY()
	TMap<FString, FKzSerializedData> ComponentData;
};

/** The global save game object that holds the state of the world. */
UCLASS()
class KZGAMEPLAY_API UKzSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Holds the binary state of every saveable actor, mapped by their Unique Save ID. */
	UPROPERTY(BlueprintReadWrite, Category = "KzGameplay|SaveSystem")
	TMap<FGuid, FKzActorSaveRecord> SavedActorsState;

	/** Keeps track of actors that were permanently destroyed (e.g., collected items, broken walls). */
	UPROPERTY(BlueprintReadWrite, Category = "KzGameplay|SaveSystem")
	TSet<FGuid> DestroyedActors;

	/** Holds raw binary data mapped by a unique identifier, useful for non-actor specific data. */
	UPROPERTY(BlueprintReadWrite, Category = "KzGameplay|SaveSystem")
	TMap<FGuid, FKzSerializedData> CustomData;
};