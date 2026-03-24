// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/KzComponentReference.h"
#include "KzSaveComponent.generated.h"

/** Delegate to notify Blueprints and other components that data has been loaded. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKzOnSaveStateRestoredSignature);

/** Defines what parts of the owner Actor should be serialized into the save file. */
UENUM(BlueprintType)
enum class EKzSaveTarget : uint8
{
	ActorOnly             UMETA(DisplayName = "Actor Only"),
	ActorAndComponents    UMETA(DisplayName = "Actor and All Components"),
	SpecificComponents    UMETA(DisplayName = "Specific Components Only")
};

/**
 * Component that marks an Actor to be tracked by the KzSaveSubsystem.
 * Automatically generates a persistent Unique ID and handles state restoration events.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzSaveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzSaveComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** Unique identifier for this specific instance in the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save System")
	FGuid UniqueSaveID;

	/** Defines what parts of the owner should be serialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save System")
	EKzSaveTarget SaveTarget = EKzSaveTarget::ActorOnly;

	/** List of specific components to save if SaveTarget is set to SpecificComponents. Pure reference via meta tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save System", meta = (EditCondition = "SaveTarget == EKzSaveTarget::SpecificComponents"))
	TSet<FKzComponentReference> ComponentsToSave;

	/** Event fired when the actor's state has been successfully loaded from the save file. */
	UPROPERTY(BlueprintAssignable, Category = "Save System")
	FKzOnSaveStateRestoredSignature OnStateRestored;

	/** True if the state has already been injected by the subsystem (e.g., during level load). */
	UPROPERTY(BlueprintReadOnly, Category = "Save System")
	bool bHasBeenRestored = false;

	/** Manually triggers the restoration event and updates the flag. */
	void NotifyStateRestored();

#if WITH_EDITOR
	virtual void OnComponentCreated() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
#endif
};