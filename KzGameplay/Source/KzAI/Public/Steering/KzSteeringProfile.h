// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KzSteeringProfile.generated.h"

class UKzSteeringBehavior;

/**
 * A Data Asset template containing a pre-configured list of steering behaviors.
 * This is duplicated into the component at runtime to avoid GC churn.
 */
UCLASS(BlueprintType, Const)
class KZAI_API UKzSteeringProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The template behaviors. Designers can add and configure them directly in the Editor. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Steering")
	TArray<TObjectPtr<UKzSteeringBehavior>> Behaviors;
};