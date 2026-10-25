// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KzGraph.h"
#include "UObject/SoftObjectPtr.h"
#include "KzComboGraph.generated.h"

class AActor;
class UKzItemDefinition;
class USkeletalMesh;

/** A combo: which attack follows which, by the input pressed and whether the attack hit. Shared by everyone who plays it; each play keeps its own count. */
UCLASS(BlueprintType)
class KZGAMEPLAY_API UKzComboGraph : public UKzGraph
{
	GENERATED_BODY()

public:
	/** The inputs that start and chain attacks, such as Input.Attack, or Input.Attack.Light and Input.Attack.Heavy. Each gives the Entry an output, and each attack one per result. */
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (Categories = "Input"))
	TArray<FGameplayTag> Inputs;

	/** How long before a combo window opens a press is kept, to chain as soon as it opens. Zero takes only presses inside the window. */
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (ClampMin = "0", Units = "Seconds"))
	float InputBuffer = 0.15f;

#if WITH_EDITORONLY_DATA
	/** The character the editor previews attacks on: its mesh, and its database for By Query animations. */
	UPROPERTY(EditAnywhere, Category = "Preview")
	TSoftClassPtr<AActor> PreviewCharacter;

	/** A mesh shown in place of the character's own, or alone without one. */
	UPROPERTY(EditAnywhere, Category = "Preview")
	TSoftObjectPtr<USkeletalMesh> PreviewMesh;

	/** The weapon the preview attacks with, whose Database fragment By Query animations ask. */
	UPROPERTY(EditAnywhere, Category = "Preview")
	TSoftObjectPtr<UKzItemDefinition> PreviewItem;
#endif

#if WITH_EDITOR
	virtual const UScriptStruct* GetNodeStruct() const override;
	virtual int32 GetMaxLinksPerOutput() const override { return 1; }
	virtual void GetDefaultNodes(TArray<const UScriptStruct*>& OutNodes) const override;
	virtual void ValidateGraph(const FKzGraphData& InData, TArray<FKzValidationIssue>& OutIssues) const override;
#endif
};