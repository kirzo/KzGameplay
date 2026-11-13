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

	/** The ability that implements the capability, or the parent of a new one. None for a capability that needs no ability. */
	UPROPERTY(EditAnywhere, Category = "Capability", meta = (AllowAbstract = "true"))
	TSubclassOf<UKzGameplayAbility> AbilityClass;

	/** Create GA_<Name> as a child of Ability Class. Off, the capability uses Ability Class itself. */
	UPROPERTY(EditAnywhere, Category = "Capability")
	bool bCreateBlueprint = true;

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