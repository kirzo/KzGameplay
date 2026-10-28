// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KzGraph.h"
#include "Scoring/KzTargetScoringProfile.h"
#include "UObject/SoftObjectPtr.h"
#include "KzComboGraph.generated.h"

class AActor;
class UKzItemDefinition;
class USkeletalMesh;

/** A combo: which attack follows which, by the input pressed and whether the attack hit. Shared by everyone who plays it; each play keeps its own count. */
UCLASS(BlueprintType, meta = (DisplayName = "Combo Graph"))
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

	/** How far to each side of the direction pressed a target is taken. */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float TargetAngle = 60.0f;

	/** Picks among the targets inside that angle; empty takes the closest. */
	UPROPERTY(EditAnywhere, Category = "Targeting")
	FKzTargetScoringProfile TargetScoring;

	/** How fast attacks turn to their target; zero turns at once. */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float TurnRate = 0.0f;

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

	/** Where in each Combo Window the preview of a whole combo presses to chain: 0 as it opens, the fastest combo; 1 as it closes, the slowest. */
	UPROPERTY(EditAnywhere, Category = "Preview", meta = (ClampMin = "0", ClampMax = "1"))
	float PreviewChainTiming = 0.0f;
#endif

#if WITH_EDITOR
	virtual const UScriptStruct* GetNodeStruct() const override;
	virtual int32 GetMaxLinksPerOutput() const override { return 1; }
	virtual void GetDefaultNodes(TArray<const UScriptStruct*>& OutNodes) const override;
	virtual void ValidateGraph(const FKzGraphData& InData, TArray<FKzValidationIssue>& OutIssues) const override;

	/** An animation asset dropped on the graph, such as a montage, makes an attack that plays it. */
	virtual bool MakeNodeForAsset(UObject& Asset, TInstancedStruct<FKzGraphNode>& OutNode) const override;
#endif
};