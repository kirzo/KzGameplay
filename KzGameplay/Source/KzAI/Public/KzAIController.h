// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagAssetInterface.h"
#include "KzAIController.generated.h"

class UKzStateTreeComponent;

/**
 * Base AI Controller for the KzAI framework.
 * Contains the execution logic for the StateTree component.
 */
UCLASS()
class KZAI_API AKzAIController : public AAIController, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AKzAIController(const FObjectInitializer& ObjectInitializer);

	/** Returns the attached StateTree Component. */
	UFUNCTION(BlueprintPure, Category = "KzAI")
	UKzStateTreeComponent* GetStateTreeComponent() const { return StateTreeComponent; }

	/** PawnTags, for whoever asks what the pawn this controller drives is, such as a sensor telling enemies from allies. */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = PawnTags; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KzAI")
	TObjectPtr<UKzStateTreeComponent> StateTreeComponent;

	/** What the pawn it drives is, such as Character.Enemy or Character.NPC. The same pawn can be an ally or an enemy by who drives it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KzAI")
	FGameplayTagContainer PawnTags;
};