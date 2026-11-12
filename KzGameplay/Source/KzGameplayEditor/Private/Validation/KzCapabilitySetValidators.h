// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzCapabilitySetValidators.generated.h"

/** Reports a parent chain that loops, and abilities listening to an input that no Input Profile of the set declares. */
UCLASS()
class UKzCapabilitySetValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};