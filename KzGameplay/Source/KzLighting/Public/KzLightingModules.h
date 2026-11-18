// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "KzLightingModules.generated.h"

class AKzLightingRig;
class USceneComponent;
class UMaterialInterface;
class UMaterialParameterCollection;

/**
 * One kind of lighting setting a profile carries, such as the sun or the fog. In the layers above the base it replaces the module of its own struct.
 * The rig blends the modules of a kind from the base up, by layer weight, and applies the result.
 */
USTRUCT(meta = (ShowIcon = "true"))
struct KZLIGHTING_API FKzLightingModule
{
	GENERATED_BODY()

	virtual ~FKzLightingModule() = default;

	/** Blends Other, a module of the same Struct, into this one. By default with Kz::LerpStruct. */
	virtual void Blend(const UScriptStruct& Struct, const FKzLightingModule& Other, float Alpha);

	/** Writes the module into the rig or the world. */
	virtual void Apply(AKzLightingRig& Rig) const {}

	/** The rig's component this module drives, which shows only while a module of its kind is in use. */
	virtual USceneComponent* GetComponent(const AKzLightingRig& Rig) const { return nullptr; }
};

/** The rig's sun, a directional light. */
USTRUCT(meta = (DisplayName = "Sun"))
struct KZLIGHTING_API FKzLightingModule_Sun : public FKzLightingModule
{
	GENERATED_BODY()

	/** The light's world rotation: where the sun shines from. */
	UPROPERTY(EditAnywhere, Category = "Sun")
	FRotator Rotation = FRotator(-45.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (ClampMin = "0", Units = "Lux"))
	float Intensity = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (HideAlphaChannel))
	FColor LightColor = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (InlineEditConditionToggle))
	bool bUseTemperature = false;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (EditCondition = "bUseTemperature", UIMin = "1700", UIMax = "12000", Units = "Kelvin"))
	float Temperature = 6500.0f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (UIMin = "0", UIMax = "6"))
	float IndirectLightingIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (UIMin = "0", UIMax = "4"))
	float VolumetricScatteringIntensity = 1.0f;

	/** Size of the sun disk: bigger gives softer shadows. */
	UPROPERTY(EditAnywhere, Category = "Sun", meta = (UIMin = "0", UIMax = "5", Units = "Degrees"))
	float LightSourceAngle = 0.5357f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (HideAlphaChannel))
	FLinearColor AtmosphereSunDiskColorScale = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Sun")
	TObjectPtr<UMaterialInterface> LightFunctionMaterial = nullptr;

	virtual void Apply(AKzLightingRig& Rig) const override;
	virtual USceneComponent* GetComponent(const AKzLightingRig& Rig) const override;
};

/** The rig's sky light, which captures the sky in real time. */
USTRUCT(meta = (DisplayName = "Sky Light"))
struct KZLIGHTING_API FKzLightingModule_SkyLight : public FKzLightingModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Sky Light", meta = (UIMin = "0", UIMax = "10"))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Light", meta = (HideAlphaChannel))
	FColor LightColor = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Sky Light", meta = (UIMin = "0", UIMax = "6"))
	float IndirectLightingIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Light", meta = (UIMin = "0", UIMax = "4"))
	float VolumetricScatteringIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Light", meta = (HideAlphaChannel))
	FLinearColor LowerHemisphereColor = FLinearColor::Black;

	virtual void Apply(AKzLightingRig& Rig) const override;
	virtual USceneComponent* GetComponent(const AKzLightingRig& Rig) const override;
};

/** The rig's exponential height fog, volumetric fog included. */
USTRUCT(meta = (DisplayName = "Height Fog"))
struct KZLIGHTING_API FKzLightingModule_HeightFog : public FKzLightingModule
{
	GENERATED_BODY()

	/** Where the fog sits, above the rig. */
	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (Units = "Centimeters"))
	float Height = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (UIMin = "0", UIMax = "0.05"))
	float FogDensity = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (UIMin = "0.001", UIMax = "2"))
	float FogHeightFalloff = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Height Fog")
	FExponentialHeightFogData SecondFogData;

	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (HideAlphaChannel))
	FLinearColor FogInscatteringLuminance = FLinearColor::Black;

	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (HideAlphaChannel))
	FLinearColor SkyAtmosphereAmbientContributionColorScale = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (UIMin = "0", UIMax = "1"))
	float FogMaxOpacity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (UIMin = "0", UIMax = "5000", Units = "Centimeters"))
	float StartDistance = 0.0f;

	/** Past this distance there is no fog. 0 means no limit. */
	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (UIMin = "0", Units = "Centimeters"))
	float FogCutoffDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Directional Inscattering", meta = (HideAlphaChannel))
	FLinearColor DirectionalInscatteringLuminance = FLinearColor::Black;

	UPROPERTY(EditAnywhere, Category = "Directional Inscattering", meta = (UIMin = "2", UIMax = "64"))
	float DirectionalInscatteringExponent = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Directional Inscattering", meta = (UIMin = "0", Units = "Centimeters"))
	float DirectionalInscatteringStartDistance = 10000.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog")
	bool bEnableVolumetricFog = false;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", UIMin = "-0.9", UIMax = "0.9"))
	float VolumetricFogScatteringDistribution = 0.2f;

	/** Keep it close to white. */
	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", HideAlphaChannel))
	FColor VolumetricFogAlbedo = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", HideAlphaChannel))
	FLinearColor VolumetricFogEmissive = FLinearColor::Black;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", UIMin = "0.1", UIMax = "10"))
	float VolumetricFogExtinctionScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", UIMin = "1000", UIMax = "10000", Units = "Centimeters"))
	float VolumetricFogDistance = 6000.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", UIMin = "0", UIMax = "10000", Units = "Centimeters"))
	float VolumetricFogStartDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Fog", meta = (EditCondition = "bEnableVolumetricFog", UIMin = "0", UIMax = "10000", Units = "Centimeters"))
	float VolumetricFogNearFadeInDistance = 0.0f;

	virtual void Apply(AKzLightingRig& Rig) const override;
	virtual USceneComponent* GetComponent(const AKzLightingRig& Rig) const override;
};

/** The rig's sky atmosphere. Defaults to the Earth's. */
USTRUCT(meta = (DisplayName = "Sky Atmosphere"))
struct KZLIGHTING_API FKzLightingModule_SkyAtmosphere : public FKzLightingModule
{
	GENERATED_BODY()

	FKzLightingModule_SkyAtmosphere();

	UPROPERTY(EditAnywhere, Category = "Planet", meta = (UIMin = "1", UIMax = "7000", Units = "Kilometers"))
	float BottomRadius = 6360.0f;

	UPROPERTY(EditAnywhere, Category = "Planet", meta = (HideAlphaChannel))
	FColor GroundAlbedo = FColor(170, 170, 170);

	UPROPERTY(EditAnywhere, Category = "Atmosphere", meta = (UIMin = "10", UIMax = "200", Units = "Kilometers"))
	float AtmosphereHeight = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere", meta = (UIMin = "0", UIMax = "1"))
	float MultiScatteringFactor = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Rayleigh", meta = (UIMin = "0", UIMax = "2"))
	float RayleighScatteringScale = 0.0331f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Rayleigh", meta = (HideAlphaChannel))
	FLinearColor RayleighScattering;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Rayleigh", meta = (UIMin = "0.01", UIMax = "20", Units = "Kilometers"))
	float RayleighExponentialDistribution = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Mie", meta = (UIMin = "0", UIMax = "5"))
	float MieScatteringScale = 0.003996f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Mie", meta = (HideAlphaChannel))
	FLinearColor MieScattering = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Mie", meta = (UIMin = "0", UIMax = "5"))
	float MieAbsorptionScale = 0.000444f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Mie", meta = (HideAlphaChannel))
	FLinearColor MieAbsorption = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Mie", meta = (UIMin = "0", UIMax = "0.999"))
	float MieAnisotropy = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Mie", meta = (UIMin = "0.01", UIMax = "10", Units = "Kilometers"))
	float MieExponentialDistribution = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Absorption", meta = (UIMin = "0", UIMax = "0.2"))
	float OtherAbsorptionScale;

	UPROPERTY(EditAnywhere, Category = "Atmosphere - Absorption", meta = (HideAlphaChannel))
	FLinearColor OtherAbsorption;

	UPROPERTY(EditAnywhere, Category = "Art Direction", meta = (HideAlphaChannel))
	FLinearColor SkyLuminanceFactor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Art Direction", meta = (HideAlphaChannel))
	FLinearColor SkyAndAerialPerspectiveLuminanceFactor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Art Direction", meta = (UIMin = "0", UIMax = "3"))
	float AerialPespectiveViewDistanceScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Art Direction", meta = (UIMin = "0", UIMax = "1"))
	float HeightFogContribution = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Art Direction", meta = (UIMin = "0.001", UIMax = "10", Units = "Kilometers"))
	float AerialPerspectiveStartDepth = 0.1f;

	virtual void Apply(AKzLightingRig& Rig) const override;
	virtual USceneComponent* GetComponent(const AKzLightingRig& Rig) const override;
};

/** The rig's volumetric clouds. */
USTRUCT(meta = (DisplayName = "Volumetric Cloud"))
struct KZLIGHTING_API FKzLightingModule_VolumetricCloud : public FKzLightingModule
{
	GENERATED_BODY()

	/** Swaps at the middle of a blend. Empty keeps the component's. */
	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (UIMin = "-50", UIMax = "50", Units = "Kilometers"))
	float LayerBottomAltitude = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (UIMin = "0.1", UIMax = "20", Units = "Kilometers"))
	float LayerHeight = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (HideAlphaChannel))
	FColor GroundAlbedo = FColor(170, 170, 170);

	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (UIMin = "0", UIMax = "1"))
	float SkyLightCloudBottomOcclusion = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (UIMin = "100", UIMax = "500", Units = "Kilometers"))
	float TracingStartMaxDistance = 350.0f;

	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (UIMin = "1", UIMax = "500", Units = "Kilometers"))
	float TracingMaxDistance = 50.0f;

	virtual void Apply(AKzLightingRig& Rig) const override;
	virtual USceneComponent* GetComponent(const AKzLightingRig& Rig) const override;
};

/** Values of a material parameter collection, so materials can follow the lighting. Each parameter blends on its own; one a side lacks blends from the collection's default. */
USTRUCT(meta = (DisplayName = "Material Parameters"))
struct KZLIGHTING_API FKzLightingModule_MaterialParameters : public FKzLightingModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Material Parameters")
	TObjectPtr<UMaterialParameterCollection> Collection = nullptr;

	UPROPERTY(EditAnywhere, Category = "Material Parameters")
	TMap<FName, float> Scalars;

	UPROPERTY(EditAnywhere, Category = "Material Parameters")
	TMap<FName, FLinearColor> Vectors;

	virtual void Blend(const UScriptStruct& Struct, const FKzLightingModule& Other, float Alpha) override;
	virtual void Apply(AKzLightingRig& Rig) const override;
};