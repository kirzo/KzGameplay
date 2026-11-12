// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzItemDefinitionValidators.generated.h"

/** Reports an Equippable fragment whose slots no Equipment Layout defines, or whose capabilities no Capability Set defines. */
UCLASS()
class UKzItemDefinitionValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};