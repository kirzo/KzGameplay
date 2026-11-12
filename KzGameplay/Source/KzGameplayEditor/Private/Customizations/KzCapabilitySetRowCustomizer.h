// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"
#include "KzCapabilitySetRowCustomizer.generated.h"

class UKzGameplayAbility;

/** What the New Capability dialog asks. Everything else is named after the capability and reused when it already exists. */
UCLASS()
class UKzNewCapabilityOptions : public UObject
{
	GENERATED_BODY()

public:
	UKzNewCapabilityOptions();

	/** Dodge gives Capability.Dodge, GA_Dodge and, with an input, Input.Dodge, IA_Dodge, IMC_Dodge and IP_Dodge. */
	UPROPERTY(EditAnywhere, Category = "Capability")
	FString Name;

	/** Whether the owner starts with the capability, or has to be given it. */
	UPROPERTY(EditAnywhere, Category = "Capability")
	bool bGrantedByDefault = true;

	/** Parent of the ability Blueprint that implements the capability. None for a capability that needs no ability. */
	UPROPERTY(EditAnywhere, Category = "Capability", meta = (AllowAbstract = "true"))
	TSubclassOf<UKzGameplayAbility> AbilityClass;

	/** Whether the capability comes with a button: its input tag, action, mapping context and profile, and the ability bound to it. */
	UPROPERTY(EditAnywhere, Category = "Capability")
	bool bHasInput = true;
};

/** Toolbar tool of the capability set editor: New Capability, which creates and wires what a capability is made of. */
class FKzCapabilitySetRowCustomizer : public FKzPropertyStackRowCustomizer
{
public:
	virtual void OnRegister(FAssetEditorToolkit* InHostEditor) override;
};