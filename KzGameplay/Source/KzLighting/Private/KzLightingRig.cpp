// Copyright 2026 kirzo

#include "KzLightingRig.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KzLightingModules.h"
#include "KzLightingVolume.h"
#include "UObject/ConstructorHelpers.h"

AKzLightingRig::AKzLightingRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// All hidden until a profile in use brings their module.
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetVisibility(false);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetVisibility(false);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetVisibility(false);

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);
	SkyAtmosphere->SetVisibility(false);

	VolumetricCloud = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	VolumetricCloud->SetupAttachment(RootComponent);
	VolumetricCloud->SetVisibility(false);

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzGameplay/EditorResources/LightingRig"));
		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Lighting");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("SpriteCategory", "Lighting", "Lighting");
		SpriteComponent->SetupAttachment(RootComponent);
	}

	SunArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("SunArrow"));
	if (SunArrow)
	{
		SunArrow->ArrowColor = FColor(255, 214, 102);
		SunArrow->bTreatAsASprite = true;
		SunArrow->bLightAttachment = true;
		SunArrow->bIsScreenSizeScaled = true;
		SunArrow->SpriteInfo.Category = TEXT("Lighting");
		SunArrow->SpriteInfo.DisplayName = NSLOCTEXT("SpriteCategory", "Lighting", "Lighting");
		SunArrow->SetupAttachment(Sun);
	}
#endif
}

AKzLightingRig* AKzLightingRig::Get(const UObject* WorldContextObject)
{
	AKzLightingRig* Found = nullptr;
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(WorldContextObject))
	{
		Registry->ForEach<AKzLightingRig>([&Found](AKzLightingRig* Rig) { Found = Rig; });
	}
	return Found;
}

void AKzLightingRig::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	bDirty = true;
	bPendingSnap = true;

	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
	ObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &AKzLightingRig::OnObjectPropertyChanged);
#endif
}

void AKzLightingRig::UnregisterAllComponents(bool bForReregister)
{
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
#endif

	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AKzLightingRig::BeginPlay()
{
	Super::BeginPlay();

	bPendingSnap = true;
}

void AKzLightingRig::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	// Layer profiles live outside any UPROPERTY: one pushed from Blueprint may have nothing else holding it.
	CastChecked<AKzLightingRig>(InThis)->Layers.ForEach([&Collector](FLayer& Layer)
	{
		Collector.AddReferencedObject(Layer.Profile);
	});
}

void AKzLightingRig::PushLayer(UObject* Source, const FKzLightingLayerSettings& Settings, bool bSnap)
{
	FLayer Layer;
	Layer.Profile = Settings.Profile;
	Layers.Push(Source, MoveTemp(Layer), Settings.Priority, Settings.bOverrideBlendTime ? Settings.BlendTime : DefaultBlendTime, false, bSnap);
}

void AKzLightingRig::PopLayer(UObject* Source, bool bSnap)
{
	Layers.Pop(Source, bSnap);
}

void AKzLightingRig::SetLayerAlpha(UObject* Source, float Alpha)
{
	Layers.SetAlpha(Source, Alpha);
}

void AKzLightingRig::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// The editor previews where the lighting settles, without time blends.
	const bool bSnap = bPendingSnap || !GetWorld()->IsGameWorld();

	FVector Location;
	if (GetViewpoint(Location))
	{
		UpdateVolumes(Location, bSnap);
		bPendingSnap = false;
	}
	Layers.Update(DeltaTime, bSnap);

	const TArray<FLayer*> OrderedLayers = Layers.GetOrdered();

	TArray<TPair<TObjectKey<UKzLightingProfile>, float>> State;
	State.Emplace(Profile.Get(), 1.0f);
	for (const FLayer* Layer : OrderedLayers)
	{
		State.Emplace(Layer->Profile.Get(), Layer->Weight);
	}

	if (bDirty || State != AppliedState)
	{
		Apply(OrderedLayers);
		AppliedState = MoveTemp(State);
		bDirty = false;
	}

	DrawDebug(OrderedLayers);
}

bool AKzLightingRig::GetViewpoint(FVector& OutLocation) const
{
	const UWorld* World = GetWorld();
	if (World->IsGameWorld() && Viewpoint == EKzLightingViewpoint::PlayerPawn)
	{
		const APlayerController* PlayerController = World->GetFirstPlayerController();
		const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!Pawn) return false;

		OutLocation = Pawn->GetActorLocation();
		return true;
	}

	// What was rendered last frame: the game's view, or the editor's perspective viewport.
	if (World->ViewLocationsRenderedLastFrame.IsEmpty()) return false;

	OutLocation = World->ViewLocationsRenderedLastFrame[0];
	return true;
}

void AKzLightingRig::UpdateVolumes(const FVector& Location, bool bSnap)
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	Registry->ForEach<AKzLightingVolume>([&](AKzLightingVolume* Volume)
	{
		const float Alpha = Volume->bUnbound ? 1.0f : Volume->GetAlpha(Location);
		const bool bActive = Layers.IsActive(Volume);

		// A progressive volume already blends in space, so it adds no time blend on top.
		const bool bVolumeSnap = bSnap || (Volume->bProgressive && !Volume->bUnbound);

		if (Alpha > 0.0f)
		{
			if (!bActive)
			{
				PushLayer(Volume, Volume->Layer, bVolumeSnap);
			}
			SetLayerAlpha(Volume, Alpha);
		}
		else if (bActive)
		{
			PopLayer(Volume, bVolumeSnap);
		}
	});
}

void AKzLightingRig::Apply(const TArray<FLayer*>& OrderedLayers)
{
	// Every kind of module the base or a layer has. A layer blending out still counts, so its kinds settle back on the base.
	TArray<const UScriptStruct*, TInlineAllocator<8>> Kinds;
	auto AddKinds = [&Kinds](const UKzLightingProfile* InProfile)
	{
		if (!InProfile) return;

		for (const FInstancedStruct& Module : InProfile->Modules)
		{
			if (const UScriptStruct* Struct = Module.GetScriptStruct())
			{
				Kinds.AddUnique(Struct);
			}
		}
	};

	AddKinds(Profile);
	for (const FLayer* Layer : OrderedLayers)
	{
		AddKinds(Layer->Profile);
	}

	TArray<USceneComponent*, TInlineAllocator<8>> Driven;
	for (const UScriptStruct* Kind : Kinds)
	{
		// A kind the base lacks starts from the module's defaults.
		FInstancedStruct Result;
		const FInstancedStruct* BaseModule = Profile ? Profile->FindModule(Kind) : nullptr;
		Result.InitializeAs(Kind, BaseModule ? BaseModule->GetMemory() : nullptr);
		FKzLightingModule& Module = Result.GetMutable<FKzLightingModule>();

		for (const FLayer* Layer : OrderedLayers)
		{
			const FInstancedStruct* LayerModule = Layer->Profile ? Layer->Profile->FindModule(Kind) : nullptr;
			if (LayerModule && Layer->Weight > 0.0f)
			{
				Module.Blend(*Kind, LayerModule->Get<FKzLightingModule>(), Layer->Weight);
			}
		}

		Module.Apply(*this);

		if (USceneComponent* Component = Module.GetComponent(*this))
		{
			Driven.AddUnique(Component);
		}
	}

	// The rig's own parts show only while a module drives them.
	USceneComponent* const Parts[] = { Sun.Get(), SkyLight.Get(), HeightFog.Get(), SkyAtmosphere.Get(), VolumetricCloud.Get() };
	for (USceneComponent* Part : Parts)
	{
		if (Part)
		{
			Part->SetVisibility(Driven.Contains(Part));
		}
	}

	ApplyPostProcess(OrderedLayers);
}

void AKzLightingRig::ApplyPostProcess(const TArray<FLayer*>& OrderedLayers)
{
	TArray<TPair<UKzLightingProfile*, float>, TInlineAllocator<8>> InUse;
	if (Profile)
	{
		InUse.Emplace(Profile, 1.0f);
	}
	for (const FLayer* Layer : OrderedLayers)
	{
		if (Layer->Profile && Layer->Weight > 0.0f)
		{
			InUse.Emplace(Layer->Profile, Layer->Weight);
		}
	}

	for (int32 Index = 0; Index < InUse.Num(); ++Index)
	{
		if (!PostProcessComponents.IsValidIndex(Index))
		{
			PostProcessComponents.AddDefaulted();
			PostProcessProfiles.AddDefaulted();
		}

		TObjectPtr<UPostProcessComponent>& Component = PostProcessComponents[Index];
		if (!Component)
		{
			Component = NewObject<UPostProcessComponent>(this, NAME_None, RF_Transient);
			Component->bUnbound = true;
			Component->RegisterComponent();
			PostProcessProfiles[Index] = nullptr;
		}

		UKzLightingProfile* InUseProfile = InUse[Index].Key;
		if (bDirty || PostProcessProfiles[Index] != InUseProfile)
		{
			Component->Settings = InUseProfile->PostProcess;
			PostProcessProfiles[Index] = InUseProfile;
		}

		// The engine blends them by priority, as it does post process volumes.
		Component->Priority = static_cast<float>(Index);
		Component->BlendWeight = InUse[Index].Value;
		Component->bEnabled = true;
	}

	for (int32 Index = InUse.Num(); Index < PostProcessComponents.Num(); ++Index)
	{
		if (PostProcessComponents[Index])
		{
			PostProcessComponents[Index]->bEnabled = false;
		}
	}
}

void AKzLightingRig::DrawDebug(const TArray<FLayer*>& OrderedLayers) const
{
	if (!bDrawDebug || !GEngine || !GetWorld()->IsGameWorld()) return;

	// Newer messages show on top, so the highest layer ends up first.
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.0f, FColor::Cyan, FString::Printf(TEXT("Lighting base: %s"), *GetNameSafe(Profile)));
	for (const FLayer* Layer : OrderedLayers)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.0f, FColor::Cyan, FString::Printf(TEXT("  %s  %.2f  (%s)"), *GetNameSafe(Layer->Profile), Layer->Weight, *GetNameSafe(Layer->Source.Get())));
	}
}

#if WITH_EDITOR
void AKzLightingRig::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (Object && Object->IsA<UKzLightingProfile>())
	{
		bDirty = true;
	}
	else if (AKzLightingVolume* Volume = Cast<AKzLightingVolume>(Object))
	{
		// Pushed again next frame with its new settings.
		Layers.Pop(Volume, true);
	}
}
#endif