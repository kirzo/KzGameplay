// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Containers/KzBlendLayers.h"
#include "GameFramework/Actor.h"
#include "KzLightingProfile.h"
#include "UObject/ObjectKey.h"
#include "KzLightingRig.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;

/** Where lighting volumes are checked from in game. The editor always uses its viewport. */
UENUM(BlueprintType)
enum class EKzLightingViewpoint : uint8
{
	/** The first local player's pawn, as camera volumes do. */
	PlayerPawn,

	/** The view, as post process volumes do. */
	Camera
};

/**
 * The lighting of a level: give it a profile and it lights the level with its sun, sky light, height fog, sky atmosphere and volumetric cloud.
 * Each of them shows only while a profile in use has its module. On top of the base profile go the layers that lighting volumes and Blueprint add.
 * For each kind of module it blends the base and the layers by weight and applies the result, only on the frames something changed.
 * The engine blends the post process settings, through one post process component per profile in use.
 * It runs in the editor as a preview, so profiles can be tuned live. One per world.
 */
UCLASS(HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZLIGHTING_API AKzLightingRig : public AActor
{
	GENERATED_BODY()

public:
	AKzLightingRig();

	/** The level's look, under every layer. Give it every kind of module the layers bring, so they blend back to it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Rig")
	TObjectPtr<UKzLightingProfile> Profile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Rig")
	EKzLightingViewpoint Viewpoint = EKzLightingViewpoint::PlayerPawn;

	/** For layers that don't set their own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Rig", meta = (ClampMin = "0", Units = "Seconds"))
	float DefaultBlendTime = 1.0f;

	/** Prints the base profile and every layer with its weight, in game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Rig|Debug")
	bool bDrawDebug = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UVolumetricCloudComponent> VolumetricCloud;

	/** The world's lighting rig, if there is one. */
	UFUNCTION(BlueprintPure, Category = "Lighting Rig", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Lighting Rig"))
	static AKzLightingRig* Get(const UObject* WorldContextObject);

	/** Adds a layer, or updates the one Source already has. Snap skips the blend. */
	UFUNCTION(BlueprintCallable, Category = "Lighting Rig", meta = (AdvancedDisplay = "bSnap"))
	void PushLayer(UObject* Source, const FKzLightingLayerSettings& Settings, bool bSnap = false);

	/** Blends out the layer of Source and removes it. */
	UFUNCTION(BlueprintCallable, Category = "Lighting Rig", meta = (AdvancedDisplay = "bSnap"))
	void PopLayer(UObject* Source, bool bSnap = false);

	/** Scales the layer of Source, as a volume does with its alpha. */
	UFUNCTION(BlueprintCallable, Category = "Lighting Rig")
	void SetLayerAlpha(UObject* Source, float Alpha);

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;

	/** Where the sun shines to. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> SunArrow = nullptr;
#endif

private:
	struct FLayer : Kz::FBlendLayer
	{
		TObjectPtr<UKzLightingProfile> Profile;
	};

	Kz::TBlendLayers<FLayer> Layers;

	/** One per profile in use, from the base up. */
	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<UPostProcessComponent>> PostProcessComponents;

	/** The profile whose settings each post process component holds. */
	TArray<TObjectKey<UKzLightingProfile>> PostProcessProfiles;

	/** The base and every layer, with their weights, as last applied. */
	TArray<TPair<TObjectKey<UKzLightingProfile>, float>> AppliedState;

	/** A profile changed in the editor: apply even if the layers didn't. */
	bool bDirty = true;

	/** Snap the blends until there is a viewpoint, so play starts on its look. */
	bool bPendingSnap = true;

	bool GetViewpoint(FVector& OutLocation) const;

	/** Adds, scales and removes the layers of the lighting volumes for the viewpoint. */
	void UpdateVolumes(const FVector& Location, bool bSnap);

	void Apply(const TArray<FLayer*>& OrderedLayers);
	void ApplyPostProcess(const TArray<FLayer*>& OrderedLayers);
	void DrawDebug(const TArray<FLayer*>& OrderedLayers) const;

#if WITH_EDITOR
	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	FDelegateHandle ObjectPropertyChangedHandle;
#endif
};