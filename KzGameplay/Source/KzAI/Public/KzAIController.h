// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "KzAIController.generated.h"

class UKzStateTreeComponent;

/**
 * Base AI Controller for the KzAI framework.
 * Contains the execution logic for the StateTree component.
 */
UCLASS()
class KZAI_API AKzAIController : public AAIController
{
	GENERATED_BODY()

public:
	AKzAIController(const FObjectInitializer& ObjectInitializer);

	/** Returns the attached StateTree Component. */
	UFUNCTION(BlueprintPure, Category = "KzAI")
	UKzStateTreeComponent* GetStateTreeComponent() const { return StateTreeComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KzAI")
	TObjectPtr<UKzStateTreeComponent> StateTreeComponent;
};