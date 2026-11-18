// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Scene.h"
#include "StructUtils/InstancedStruct.h"
#include "KzLightingProfile.generated.h"

/**
 * A lighting look: the level's, as the rig's base, or one that a volume or Blueprint blends in on top.
 * Each module is one kind of setting (sun, sky light, fog...). A profile in a layer only changes the kinds it has.
 */
UCLASS(BlueprintType)
class KZLIGHTING_API UKzLightingProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** One per kind; if a kind repeats, the first counts. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (BaseStruct = "/Script/KzLighting.KzLightingModule", ExcludeBaseStruct))
	TArray<FInstancedStruct> Modules;

	/** Blended by the engine, as post process volumes are: only the ticked settings take part. */
	UPROPERTY(EditAnywhere, Category = "Post Process", meta = (ShowPostProcessCategories))
	FPostProcessSettings PostProcess;

	const FInstancedStruct* FindModule(const UScriptStruct* Struct) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/** A layer of lighting brought by a volume or Blueprint. */
USTRUCT(BlueprintType)
struct KZLIGHTING_API FKzLightingLayerSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	TObjectPtr<UKzLightingProfile> Profile = nullptr;

	/** Higher goes on top. Among equals, the latest activated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (InlineEditConditionToggle))
	bool bOverrideBlendTime = false;

	/** Time to blend in and out. Without it, the rig's default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bOverrideBlendTime", ClampMin = "0", Units = "Seconds"))
	float BlendTime = 1.0f;
};