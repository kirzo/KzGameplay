// Copyright 2026 kirzo

#include "KzLightingModules.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Core/KzStructLerp.h"
#include "Engine/World.h"
#include "KzLightingRig.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

void FKzLightingModule::Blend(const UScriptStruct& Struct, const FKzLightingModule& Other, float Alpha)
{
	Kz::LerpStruct(&Struct, this, &Other, Alpha);
}

USceneComponent* FKzLightingModule_Sun::GetComponent(const AKzLightingRig& Rig) const
{
	return Rig.Sun;
}

void FKzLightingModule_Sun::Apply(AKzLightingRig& Rig) const
{
	UDirectionalLightComponent* Light = Rig.Sun;
	if (!Light) return;

	Light->SetWorldRotation(Rotation);
	Light->SetIntensity(Intensity);
	Light->SetLightFColor(LightColor);
	Light->SetUseTemperature(bUseTemperature);
	Light->SetTemperature(Temperature);
	Light->SetIndirectLightingIntensity(IndirectLightingIntensity);
	Light->SetVolumetricScatteringIntensity(VolumetricScatteringIntensity);
	Light->SetLightSourceAngle(LightSourceAngle);
	Light->SetAtmosphereSunDiskColorScale(AtmosphereSunDiskColorScale);
	Light->SetLightFunctionMaterial(LightFunctionMaterial);
}

USceneComponent* FKzLightingModule_SkyLight::GetComponent(const AKzLightingRig& Rig) const
{
	return Rig.SkyLight;
}

void FKzLightingModule_SkyLight::Apply(AKzLightingRig& Rig) const
{
	USkyLightComponent* Light = Rig.SkyLight;
	if (!Light) return;

	Light->SetIntensity(Intensity);
	Light->SetLightColor(FLinearColor(LightColor));
	Light->SetIndirectLightingIntensity(IndirectLightingIntensity);
	Light->SetVolumetricScatteringIntensity(VolumetricScatteringIntensity);
	Light->SetLowerHemisphereColor(LowerHemisphereColor);
}

USceneComponent* FKzLightingModule_HeightFog::GetComponent(const AKzLightingRig& Rig) const
{
	return Rig.HeightFog;
}

void FKzLightingModule_HeightFog::Apply(AKzLightingRig& Rig) const
{
	UExponentialHeightFogComponent* Fog = Rig.HeightFog;
	if (!Fog) return;

	const FVector Location = Fog->GetRelativeLocation();
	Fog->SetRelativeLocation(FVector(Location.X, Location.Y, Height));
	Fog->SetFogDensity(FogDensity);
	Fog->SetFogHeightFalloff(FogHeightFalloff);
	Fog->SetSecondFogData(SecondFogData);
	Fog->SetFogInscatteringColor(FogInscatteringLuminance);
	Fog->SetSkyAtmosphereAmbientContributionColorScale(SkyAtmosphereAmbientContributionColorScale);
	Fog->SetFogMaxOpacity(FogMaxOpacity);
	Fog->SetStartDistance(StartDistance);
	Fog->SetFogCutoffDistance(FogCutoffDistance);
	Fog->SetDirectionalInscatteringColor(DirectionalInscatteringLuminance);
	Fog->SetDirectionalInscatteringExponent(DirectionalInscatteringExponent);
	Fog->SetDirectionalInscatteringStartDistance(DirectionalInscatteringStartDistance);
	Fog->SetVolumetricFog(bEnableVolumetricFog);
	Fog->SetVolumetricFogScatteringDistribution(VolumetricFogScatteringDistribution);
	Fog->SetVolumetricFogAlbedo(VolumetricFogAlbedo);
	Fog->SetVolumetricFogEmissive(VolumetricFogEmissive);
	Fog->SetVolumetricFogExtinctionScale(VolumetricFogExtinctionScale);
	Fog->SetVolumetricFogDistance(VolumetricFogDistance);
	Fog->SetVolumetricFogStartDistance(VolumetricFogStartDistance);
	Fog->SetVolumetricFogNearFadeInDistance(VolumetricFogNearFadeInDistance);
}

FKzLightingModule_SkyAtmosphere::FKzLightingModule_SkyAtmosphere()
{
	// As USkyAtmosphereComponent: the Earth's coefficients, kept as a color plus a scale.
	const FLinearColor RayleighScatteringRaw(0.005802f, 0.013558f, 0.033100f);
	RayleighScattering = RayleighScatteringRaw * (1.0f / RayleighScatteringRaw.B);
	RayleighScatteringScale = RayleighScatteringRaw.B;

	const FLinearColor OtherAbsorptionRaw(0.000650f, 0.001881f, 0.000085f);
	OtherAbsorptionScale = OtherAbsorptionRaw.G;
	OtherAbsorption = OtherAbsorptionRaw * (1.0f / OtherAbsorptionRaw.G);
}

USceneComponent* FKzLightingModule_SkyAtmosphere::GetComponent(const AKzLightingRig& Rig) const
{
	return Rig.SkyAtmosphere;
}

void FKzLightingModule_SkyAtmosphere::Apply(AKzLightingRig& Rig) const
{
	USkyAtmosphereComponent* Atmosphere = Rig.SkyAtmosphere;
	if (!Atmosphere) return;

	Atmosphere->SetBottomRadius(BottomRadius);
	Atmosphere->SetGroundAlbedo(GroundAlbedo);
	Atmosphere->SetAtmosphereHeight(AtmosphereHeight);
	Atmosphere->SetMultiScatteringFactor(MultiScatteringFactor);
	Atmosphere->SetRayleighScatteringScale(RayleighScatteringScale);
	Atmosphere->SetRayleighScattering(RayleighScattering);
	Atmosphere->SetRayleighExponentialDistribution(RayleighExponentialDistribution);
	Atmosphere->SetMieScatteringScale(MieScatteringScale);
	Atmosphere->SetMieScattering(MieScattering);
	Atmosphere->SetMieAbsorptionScale(MieAbsorptionScale);
	Atmosphere->SetMieAbsorption(MieAbsorption);
	Atmosphere->SetMieAnisotropy(MieAnisotropy);
	Atmosphere->SetMieExponentialDistribution(MieExponentialDistribution);
	Atmosphere->SetOtherAbsorptionScale(OtherAbsorptionScale);
	Atmosphere->SetOtherAbsorption(OtherAbsorption);
	Atmosphere->SetSkyLuminanceFactor(SkyLuminanceFactor);
	Atmosphere->SetSkyAndAerialPerspectiveLuminanceFactor(SkyAndAerialPerspectiveLuminanceFactor);
	Atmosphere->SetAerialPespectiveViewDistanceScale(AerialPespectiveViewDistanceScale);
	Atmosphere->SetHeightFogContribution(HeightFogContribution);
	Atmosphere->SetAerialPerspectiveStartDepth(AerialPerspectiveStartDepth);
}

USceneComponent* FKzLightingModule_VolumetricCloud::GetComponent(const AKzLightingRig& Rig) const
{
	return Rig.VolumetricCloud;
}

void FKzLightingModule_VolumetricCloud::Apply(AKzLightingRig& Rig) const
{
	UVolumetricCloudComponent* Cloud = Rig.VolumetricCloud;
	if (!Cloud) return;

	if (Material)
	{
		Cloud->SetMaterial(Material);
	}
	Cloud->SetLayerBottomAltitude(LayerBottomAltitude);
	Cloud->SetLayerHeight(LayerHeight);
	Cloud->SetGroundAlbedo(GroundAlbedo);
	Cloud->SetSkyLightCloudBottomOcclusion(SkyLightCloudBottomOcclusion);
	Cloud->SetTracingStartMaxDistance(TracingStartMaxDistance);
	Cloud->SetTracingMaxDistance(TracingMaxDistance);
}

void FKzLightingModule_MaterialParameters::Blend(const UScriptStruct& Struct, const FKzLightingModule& InOther, float Alpha)
{
	const FKzLightingModule_MaterialParameters& Other = static_cast<const FKzLightingModule_MaterialParameters&>(InOther);
	const UMaterialParameterCollection* Defaults = Collection ? Collection.Get() : Other.Collection.Get();

	for (const TPair<FName, float>& Pair : Other.Scalars)
	{
		if (!Scalars.Contains(Pair.Key))
		{
			const FCollectionScalarParameter* Parameter = Defaults ? Defaults->GetScalarParameterByName(Pair.Key) : nullptr;
			Scalars.Add(Pair.Key, Parameter ? Parameter->DefaultValue : 0.0f);
		}
		float& Value = Scalars[Pair.Key];
		Value = FMath::Lerp(Value, Pair.Value, Alpha);
	}

	for (const TPair<FName, FLinearColor>& Pair : Other.Vectors)
	{
		if (!Vectors.Contains(Pair.Key))
		{
			const FCollectionVectorParameter* Parameter = Defaults ? Defaults->GetVectorParameterByName(Pair.Key) : nullptr;
			Vectors.Add(Pair.Key, Parameter ? Parameter->DefaultValue : FLinearColor::Black);
		}
		FLinearColor& Value = Vectors[Pair.Key];
		Value = FMath::Lerp(Value, Pair.Value, Alpha);
	}

	if (Alpha >= 0.5f && Other.Collection)
	{
		Collection = Other.Collection;
	}
}

void FKzLightingModule_MaterialParameters::Apply(AKzLightingRig& Rig) const
{
	UWorld* World = Rig.GetWorld();
	UMaterialParameterCollectionInstance* Instance = World && Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
	if (!Instance) return;

	for (const TPair<FName, float>& Pair : Scalars)
	{
		Instance->SetScalarParameterValue(Pair.Key, Pair.Value);
	}

	for (const TPair<FName, FLinearColor>& Pair : Vectors)
	{
		Instance->SetVectorParameterValue(Pair.Key, Pair.Value);
	}
}