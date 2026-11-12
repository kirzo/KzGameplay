// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzInputProfileValidators.generated.h"

/** Reports malformed action entries on a UKzInputProfile (null action, missing tag, duplicate tag) and what its Mapping Context leaves unmatched: actions with no key, keys for actions it does not declare. */
UCLASS()
class UKzInputProfileValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};