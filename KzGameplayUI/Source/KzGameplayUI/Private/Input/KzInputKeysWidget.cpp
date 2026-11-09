// Copyright 2026 kirzo

#include "Input/KzInputKeysWidget.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputProfile.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzInputIcon.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"

namespace
{
	TInstancedStruct<FKzUIIconSource> MakeKeySource(const FKey& Key)
	{
		FKzUIIconSource_Key Source;
		Source.Key = Key;
		return TInstancedStruct<FKzUIIconSource>::Make<FKzUIIconSource_Key>(Source);
	}

	bool IsFaceButton(const FKey& Key)
	{
		return Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Gamepad_FaceButton_Left
			|| Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_FaceButton_Bottom;
	}

	/** The player's first key for Action on a pad, or on the keyboard and mouse. */
	FKey FindKey(const UEnhancedInputLocalPlayerSubsystem& EnhancedInput, const UInputAction* Action, bool bGamepad)
	{
		if (Action)
		{
			for (const FKey& Key : EnhancedInput.QueryKeysMappedToAction(Action))
			{
				if (Key.IsGamepadKey() == bGamepad)
				{
					return Key;
				}
			}
		}
		return FKey();
	}
}

void UKzInputKeyGroupWidget::SetKeys(const TArray<FKey>& Keys)
{
	if (Icon && Keys.Num() > 0)
	{
		Icon->SetSource(MakeKeySource(Keys[0]));
	}
	for (const auto& [FaceIcon, FaceKey] : GetFaceIcons())
	{
		if (FaceIcon)
		{
			FaceIcon->SetRenderOpacity(Keys.Contains(FaceKey) ? 1.0f : DimmedOpacity);
		}
	}
}

void UKzInputKeyGroupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Here so the designer shows them too
	for (const auto& [FaceIcon, FaceKey] : GetFaceIcons())
	{
		if (FaceIcon)
		{
			FaceIcon->SetSource(MakeKeySource(FaceKey));
		}
	}
}

TArray<TPair<UKzInputIcon*, FKey>, TInlineAllocator<4>> UKzInputKeyGroupWidget::GetFaceIcons() const
{
	return {
		MakeTuple(FaceTop.Get(), EKeys::Gamepad_FaceButton_Top),
		MakeTuple(FaceLeft.Get(), EKeys::Gamepad_FaceButton_Left),
		MakeTuple(FaceRight.Get(), EKeys::Gamepad_FaceButton_Right),
		MakeTuple(FaceBottom.Get(), EKeys::Gamepad_FaceButton_Bottom),
	};
}

void UKzInputKeysWidget::SetInputTag(FGameplayTag NewInputTag)
{
	InputTag = NewInputTag;
	Refresh();
}

void UKzInputKeysWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

#if WITH_EDITOR
	if (IsDesignTime())
	{
		ShowKeys(PreviewKeys);
	}
#endif
}

void UKzInputKeysWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::GetForWidget(this))
	{
		Subsystem->OnInputDeviceChanged.AddUniqueDynamic(this, &UKzInputKeysWidget::OnInputDeviceChanged);
		Subsystem->OnInputMappingsChanged.AddUniqueDynamic(this, &UKzInputKeysWidget::Refresh);
		BoundSubsystem = Subsystem;
	}
	Refresh();
}

void UKzInputKeysWidget::NativeDestruct()
{
	if (UKzUIInputSubsystem* Subsystem = BoundSubsystem.Get())
	{
		Subsystem->OnInputDeviceChanged.RemoveDynamic(this, &UKzInputKeysWidget::OnInputDeviceChanged);
		Subsystem->OnInputMappingsChanged.RemoveDynamic(this, &UKzInputKeysWidget::Refresh);
	}
	BoundSubsystem = nullptr;

	Super::NativeDestruct();
}

void UKzInputKeysWidget::Refresh()
{
	TArray<FKey> NewKeys;
	const UKzUIInputSubsystem* Subsystem = BoundSubsystem.Get();
	const UEnhancedInputLocalPlayerSubsystem* EnhancedInput = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	const APlayerController* Controller = GetOwningPlayer();
	const UKzInputHandlerComponent* Handler = Controller ? UKzSystemLibrary::FindComponentInActorOrController<UKzInputHandlerComponent>(Controller->GetPawn()) : nullptr;
	const UKzInputProfile* Profile = Handler ? Handler->FindActionProfile(InputTag) : nullptr;
	const FKzInputAction* Config = Profile ? Profile->FindActionConfigForTag(InputTag) : nullptr;
	const UInputAction* Action = Config ? Config->InputAction.Get() : nullptr;
	if (Subsystem && EnhancedInput && Action)
	{
		// Pad keys also on the keyboard when the game shows pad icons there, as KzInputIcon does
		const bool bGamepad = Subsystem->GetCurrentInputDevice() != EKzUIInputDevice::Keyboard || UKzUIInputSettings::Get()->bGamepadIconsOnKeyboard;
		const FKey Key = FindKey(*EnhancedInput, Action, bGamepad);

		auto AddChordKeys = [&](const TArray<TObjectPtr<UInputTrigger>>& Triggers)
		{
			for (const UInputTrigger* Trigger : Triggers)
			{
				const UInputTriggerChordAction* Chord = Cast<UInputTriggerChordAction>(Trigger);
				const FKey ChordKey = Chord ? FindKey(*EnhancedInput, Chord->ChordAction, bGamepad) : FKey();
				if (ChordKey.IsValid())
				{
					NewKeys.Add(ChordKey);
				}
			}
		};

		if (Key.IsValid())
		{
			// Chords on the action, then on its mapping in the profile's context: the player's own mappings aren't public
			AddChordKeys(Action->Triggers);
			const TArray<FEnhancedActionKeyMapping>* Mappings = Profile->MappingContext ? &Profile->MappingContext->GetMappings() : nullptr;
			const FEnhancedActionKeyMapping* Mapping = Mappings ? Mappings->FindByPredicate([Action, bGamepad](const FEnhancedActionKeyMapping& Each)
			{
				return Each.Action == Action && Each.Key.IsGamepadKey() == bGamepad;
			}) : nullptr;
			if (Mapping)
			{
				AddChordKeys(Mapping->Triggers);
			}
			NewKeys.Add(Key);
		}
	}

	if (NewKeys != ShownKeys)
	{
		ShowKeys(NewKeys);
	}
}

void UKzInputKeysWidget::ShowKeys(const TArray<FKey>& NewKeys)
{
	ShownKeys = NewKeys;
	if (!Keys)
	{
		return;
	}
	Keys->ClearChildren();

	auto AddToRow = [this](TSubclassOf<UUserWidget> Class)
	{
		UUserWidget* Widget = CreateWidget(this, Class);
#if WITH_EDITOR
		// So their icons preview in the designer like the row's
		Widget->SetDesignerFlags(GetDesignerFlags());
#endif
		// Groups of different heights line up on their centers
		if (UHorizontalBoxSlot* RowSlot = Cast<UHorizontalBoxSlot>(Keys->AddChild(Widget)))
		{
			RowSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Widget;
	};

	const bool bDiamond = FaceButtonStyle == EKzFaceButtonStyle::Diamond && DiamondClass;
	const TArray<FKey> FaceKeys = bDiamond ? NewKeys.FilterByPredicate(&IsFaceButton) : TArray<FKey>();
	for (const FKey& Key : NewKeys)
	{
		// All the face buttons go in the diamond of the first one
		const bool bFace = FaceKeys.Contains(Key);
		const TSubclassOf<UKzInputKeyGroupWidget> Class = bFace ? DiamondClass : KeyClass;
		if (!Class || (bFace && Key != FaceKeys[0]))
		{
			continue;
		}

		if (SeparatorClass && Keys->GetChildrenCount() > 0)
		{
			AddToRow(SeparatorClass);
		}
		CastChecked<UKzInputKeyGroupWidget>(AddToRow(Class))->SetKeys(bFace ? FaceKeys : TArray<FKey>{ Key });
	}
}
