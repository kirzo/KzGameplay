// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzInputProfileValidators.generated.h"

/** Reports malformed action entries on a UKzInputProfile: null action, missing tag, or duplicate tag. */
UCLASS()
class UKzInputProfileValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};
