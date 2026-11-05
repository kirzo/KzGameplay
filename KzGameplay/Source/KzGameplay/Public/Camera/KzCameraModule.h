// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Camera/KzCameraTypes.h"
#include "KzCameraModule.generated.h"

/**
 * A piece of camera behaviour, added to a camera rig or to a layer (volume, marker, Blueprint).
 * Its kind is the class right below UKzCameraModule: in upper layers it replaces the module of its kind,
 * so a game subclass of a Kz module replaces that module instead of adding a new kind.
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType, CollapseCategories)
class KZGAMEPLAY_API UKzCameraModule : public UObject
{
	GENERATED_BODY()

public:
	/** Returns the pose changed by this module. */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Module")
	FKzCameraPose Evaluate(const FKzCameraContext& Context, const FKzCameraPose& Pose);

	UFUNCTION(BlueprintNativeEvent, Category = "Camera Module")
	EKzCameraStage GetStage() const;

	/** Modules that remember between frames get a copy per rig, so two cameras on the same layer don't share memory. */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Module")
	bool HasState() const;

	UClass* GetSlotClass() const;

	/** Draws what the module does, with the final pose. The rig calls it while the camera is selected in the editor, or when it draws debug. */
	virtual void DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose) const {}

	virtual UWorld* GetWorld() const override;

protected:
	/** Class traits, set by each module's constructor; Blueprint modules override the getters instead. */
	EKzCameraStage Stage = EKzCameraStage::Position;
	bool bHasState = false;

	virtual FKzCameraPose Evaluate_Implementation(const FKzCameraContext& Context, const FKzCameraPose& Pose) { return Pose; }
	virtual EKzCameraStage GetStage_Implementation() const { return Stage; }
	virtual bool HasState_Implementation() const { return bHasState; }
};
