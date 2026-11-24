// Copyright 2026 kirzo

#include "Camera/KzCameraDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Actors/KzAlphaVolume.h"
#include "Camera/KzCameraDirector.h"
#include "Camera/KzCameraModule.h"
#include "Camera/KzCameraRigComponent.h"
#include "Camera/KzCameraZone.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/KzShapeComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "KzDrawDebugHelpers.h"

namespace
{
	using FLayer = UKzCameraRigComponent::FLayer;

	FString Describe(const UObject* Object)
	{
		return UKzCameraDirector::Describe(Object);
	}

	FString GetModuleName(const UClass* Class)
	{
		return Class ? FName::NameToDisplayString(Class->GetName().Replace(TEXT("KzCameraModule_"), TEXT("")), false) : FString(TEXT("None"));
	}

	float GetOwnWeight(const FLayer& Layer)
	{
		return Layer.Alpha * FMath::SmoothStep(0.0f, 1.0f, Layer.Blend);
	}

	bool HasSlot(const TArray<TObjectPtr<UKzCameraModule>>& Modules, const UClass* SlotClass)
	{
		return Modules.ContainsByPredicate([SlotClass](const UKzCameraModule* Module) { return Module && Module->GetSlotClass() == SlotClass; });
	}

	void DrawView(FGameplayDebuggerCanvasContext& CanvasContext, const UWorld& World, const APlayerCameraManager& CameraManager, const UKzCameraDirector* Director)
	{
		CanvasContext.Printf(TEXT("{green}In view {white}%s"), *Describe(CameraManager.GetViewTarget()));

		if (const AActor* Incoming = CameraManager.PendingViewTarget.Target)
		{
			const FViewTargetTransitionParams& Blend = CameraManager.BlendParams;
			const FString Function = StaticEnum<EViewTargetBlendFunction>()->GetNameStringByValue(static_cast<int64>(Blend.BlendFunction.GetValue())).Replace(TEXT("VTBlend_"), TEXT(""));
			CanvasContext.Printf(TEXT("{green}Blending to {yellow}%s {white}%.1f of %.1f s, %s"), *Describe(Incoming), Blend.BlendTime - CameraManager.BlendTimeToGo, Blend.BlendTime, *Function);
		}

		if (!Director)
		{
			CanvasContext.Printf(TEXT("{red}No camera director in this world."));
			return;
		}

		const FKzCameraEvent& LastSwitch = Director->GetLastSwitch();
		if (!LastSwitch.Text.IsEmpty())
		{
			CanvasContext.Printf(TEXT("{green}Why {white}%s {cyan}%.1f s ago"), *LastSwitch.Text, World.GetTimeSeconds() - LastSwitch.Time);
		}

		const AKzCameraZone* Zone = Director->GetCurrentZone();
		CanvasContext.Printf(TEXT("{green}Gameplay camera {white}%s   {green}Zone {white}%s"), *Describe(Director->GetGameplayCamera()), Zone ? *Describe(Zone) : TEXT("none"));

		TArray<FString> Awake;
		if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(&World))
		{
			Registry->ForEach<UKzCameraRigComponent>([&Awake](const UKzCameraRigComponent* Rig)
			{
				if (Rig->IsAwake())
				{
					Awake.Add(Describe(Rig));
				}
			});
		}
		CanvasContext.Printf(TEXT("{green}Awake {white}%s"), Awake.IsEmpty() ? TEXT("none") : *FString::Join(Awake, TEXT(", ")));
	}

	void DrawLayers(FGameplayDebuggerCanvasContext& CanvasContext, const TArray<FLayer*>& Layers, const AActor* Target)
	{
		CanvasContext.Printf(TEXT("{green}Layers{white}, top first"));
		if (Layers.IsEmpty())
		{
			CanvasContext.Printf(TEXT("  {white}None: only the rig's own modules."));
			return;
		}

		for (int32 Index = Layers.Num() - 1; Index >= 0; --Index)
		{
			const FLayer& Layer = *Layers[Index];
			TArray<FString> State;

			if (Layer.bRemoving)
			{
				State.Add(FString::Printf(TEXT("leaving %.0f%%"), Layer.Blend * 100.0f));
			}
			else if (Layer.Blend < 1.0f)
			{
				State.Add(FString::Printf(TEXT("entering %.0f%%"), Layer.Blend * 100.0f));
			}

			if (const AKzAlphaVolume* Volume = Cast<AKzAlphaVolume>(Layer.Source.Get()))
			{
				State.Add(FString::Printf(TEXT("alpha %.2f"), Layer.Alpha));
				if (!Layer.bRemoving && Target && Volume->GetAlpha(Target->GetActorLocation()) <= 0.0f)
				{
					State.Add(TEXT("kept after leaving"));
				}
			}

			// Hidden, partly or fully, by the clearing layers activated after it.
			const float OwnWeight = GetOwnWeight(Layer);
			if (OwnWeight > 0.0f && Layer.Weight < OwnWeight - KINDA_SMALL_NUMBER)
			{
				TArray<FString> Clearing;
				for (const FLayer* Other : Layers)
				{
					if (Other->bClearPrevious && Other->Order > Layer.Order && GetOwnWeight(*Other) > 0.0f)
					{
						Clearing.Add(Describe(Other->Source.Get()));
					}
				}
				State.Add(FString::Printf(TEXT("hidden by %s"), *FString::Join(Clearing, TEXT(", "))));
			}

			if (Layer.bClearPrevious)
			{
				State.Add(TEXT("clears previous"));
			}

			TArray<FString> Modules;
			for (const UKzCameraModule* Module : Layer.Modules)
			{
				if (Module)
				{
					Modules.Add(GetModuleName(Module->GetSlotClass()));
				}
			}

			CanvasContext.Printf(TEXT("  %s%s {white}priority %d, weight %.2f%s%s  {cyan}%s"), Layer.Weight > 0.0f ? TEXT("{yellow}") : TEXT("{orange}"), *Describe(Layer.Source.Get()),
				Layer.Priority, Layer.Weight, State.IsEmpty() ? TEXT("") : TEXT(", "), *FString::Join(State, TEXT(", ")), Modules.IsEmpty() ? TEXT("no modules") : *FString::Join(Modules, TEXT(", ")));
		}
	}

	void DrawModules(FGameplayDebuggerCanvasContext& CanvasContext, const UKzCameraRigComponent& Rig, const TArray<FLayer*>& Layers)
	{
		struct FSlot
		{
			UClass* Class;
			EKzCameraStage Stage;
		};

		// Every kind of module the rig or a layer has, by stage, as the rig runs them.
		TArray<FSlot, TInlineAllocator<16>> Slots;
		auto AddSlots = [&Slots](const TArray<TObjectPtr<UKzCameraModule>>& Modules)
		{
			for (const UKzCameraModule* Module : Modules)
			{
				UClass* SlotClass = Module ? Module->GetSlotClass() : nullptr;
				if (SlotClass && !Slots.ContainsByPredicate([SlotClass](const FSlot& Slot) { return Slot.Class == SlotClass; }))
				{
					Slots.Add({ SlotClass, Module->GetStage() });
				}
			}
		};

		AddSlots(Rig.Modules);
		for (const FLayer* Layer : Layers)
		{
			AddSlots(Layer->Modules);
		}
		Slots.StableSort([](const FSlot& A, const FSlot& B) { return A.Stage < B.Stage; });

		CanvasContext.Printf(TEXT("{green}Modules{white}, by stage: who sets each, over whom it blends"));
		for (const FSlot& Slot : Slots)
		{
			// From the top layer down to the first one at full weight, which hides the rest.
			TArray<FString> Sources;
			bool bCovered = false;
			for (int32 Index = Layers.Num() - 1; Index >= 0 && !bCovered; --Index)
			{
				const FLayer& Layer = *Layers[Index];
				if (Layer.Weight <= 0.0f || !HasSlot(Layer.Modules, Slot.Class)) continue;

				bCovered = Layer.Weight >= 1.0f - KINDA_SMALL_NUMBER;
				Sources.Add(bCovered ? Describe(Layer.Source.Get()) : FString::Printf(TEXT("%s %.0f%%"), *Describe(Layer.Source.Get()), Layer.Weight * 100.0f));
			}
			if (!bCovered && HasSlot(Rig.Modules, Slot.Class))
			{
				Sources.Add(TEXT("rig"));
			}

			const FString Stage = StaticEnum<EKzCameraStage>()->GetNameStringByValue(static_cast<int64>(Slot.Stage));
			CanvasContext.Printf(TEXT("  {cyan}%s {white}%s  %s"), *Stage, *GetModuleName(Slot.Class),
				Sources.IsEmpty() ? TEXT("{orange}off") : *(TEXT("{yellow}") + FString::Join(Sources, TEXT(" {white}over{yellow} "))));
		}
	}

	void DrawHistory(FGameplayDebuggerCanvasContext& CanvasContext, const UWorld& World, const UKzCameraDirector& Director)
	{
		CanvasContext.Printf(TEXT("{green}History{white}, newest first"));

		const TArray<FKzCameraEvent>& History = Director.GetHistory();
		for (int32 Index = History.Num() - 1; Index >= 0; --Index)
		{
			CanvasContext.Printf(TEXT("  {cyan}%.1f s ago {white}%s"), World.GetTimeSeconds() - History[Index].Time, *History[Index].Text);
		}
	}

	void DrawZones(UWorld& World, const UKzCameraDirector* Director)
	{
		UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(&World);
		if (!Registry) return;

		Registry->ForEach<AKzCameraZone>([&World, Director](const AKzCameraZone* Zone)
		{
			const UKzShapeComponent* Shape = Zone->ShapeComponent;
			if (!Shape || !Shape->Shape.IsValid()) return;

			const bool bCurrent = Director && Director->GetCurrentZone() == Zone;
			const FColor Color = bCurrent ? FColor::Green : FColor::Orange;
			const FVector Center = Shape->GetComponentLocation();

			DrawDebugShape(&World, Center, Shape->GetComponentQuat(), Shape->Shape * Shape->GetComponentScale(), Color, false, -1.0f, SDPG_World, bCurrent ? 3.0f : 1.5f);
			if (Zone->Camera)
			{
				DrawDebugDirectionalArrow(&World, Center, Zone->Camera->GetActorLocation(), 60.0f, Color, false, -1.0f, SDPG_World, 1.5f);
			}
			DrawDebugString(&World, Center, Describe(Zone), nullptr, Color, 0.0f);
		});
	}
}

FKzCameraDebuggerCategory::FKzCameraDebuggerCategory()
{
	bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FKzCameraDebuggerCategory::MakeInstance()
{
	return MakeShareable(new FKzCameraDebuggerCategory());
}

void FKzCameraDebuggerCategory::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	UWorld* World = CanvasContext.World.Get();
	const APlayerCameraManager* CameraManager = OwnerPC ? OwnerPC->PlayerCameraManager.Get() : nullptr;
	if (!World || !CameraManager) return;

	const UKzCameraDirector* Director = UKzCameraDirector::Get(World);
	DrawView(CanvasContext, *World, *CameraManager, Director);
	DrawZones(*World, Director);

	// The camera blending in, or else the one in view.
	AActor* Shown = CameraManager->PendingViewTarget.Target ? CameraManager->PendingViewTarget.Target.Get() : CameraManager->GetViewTarget();
	UKzCameraRigComponent* Rig = Shown ? Shown->FindComponentByClass<UKzCameraRigComponent>() : nullptr;
	if (Rig)
	{
		UKzCameraRigComponent::DebuggerFrame = GFrameCounter;

		const AActor* Target = Rig->GetTarget();
		const float FocusDistance = Target ? FVector::Dist(Rig->GetFocus(), Target->GetActorLocation()) : 0.0f;
		CanvasContext.Printf(TEXT("{green}Setup of {white}%s   {green}Target {white}%s   {green}Focus {white}%.0f cm from it"), *Describe(Rig), *Describe(Target), FocusDistance);

		const TArray<FLayer*> Layers = Rig->GetLayers();
		DrawLayers(CanvasContext, Layers, Target);
		DrawModules(CanvasContext, *Rig, Layers);
	}
	else
	{
		CanvasContext.Printf(TEXT("{yellow}%s has no camera rig."), *Describe(Shown));
	}

	if (Director)
	{
		DrawHistory(CanvasContext, *World, *Director);
	}
}
#endif