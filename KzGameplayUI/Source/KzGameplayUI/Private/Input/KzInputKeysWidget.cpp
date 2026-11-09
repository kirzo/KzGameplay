// Copyright 2026 kirzo

#include "Input/KzInputKeysWidget.h"
#include "Input/KzUIIconSource_InputTag.h"
#include "KzInputIcon.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Algo/AnyOf.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"

namespace
{
	TInstancedStruct<FKzUIIconSource> MakeKeySource(const FKey& Key)
	{
		FKzUIIconSource_Key Source;
		Source.Key = Key;
		return TInstancedStruct<FKzUIIconSource>::Make<FKzUIIconSource_Key>(Source);
	}

	/** The player's first mapping of Action to a pad key, or to a keyboard or mouse key. */
	const FEnhancedActionKeyMapping* FindMapping(const UEnhancedPlayerInput& PlayerInput, const UInputAction* Action, bool bGamepad)
	{
		if (Action)
		{
			for (const FEnhancedActionKeyMapping& Mapping : PlayerInput.GetEnhancedActionMappings())
			{
				if (Mapping.Action == Action && Mapping.Key.IsGamepadKey() == bGamepad)
				{
					return &Mapping;
				}
			}
		}
		return nullptr;
	}

	const UInputAction* FindChordAction(const FEnhancedActionKeyMapping& Mapping)
	{
		// The player's mappings also carry the action's own chord triggers. Blockers are the ones Enhanced Input adds to the mappings a chord masks
		for (const UInputTrigger* Trigger : Mapping.Triggers)
		{
			const UInputTriggerChordAction* ChordTrigger = Cast<UInputTriggerChordAction>(Trigger);
			if (ChordTrigger && !ChordTrigger->IsA<UInputTriggerChordBlocker>())
			{
				return ChordTrigger->ChordAction;
			}
		}
		return nullptr;
	}

	void SetShown(UWidget* Widget, bool bShown)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UKzInputKeysWidget::SetInputTag(FGameplayTag NewInputTag)
{
	InputTag = NewInputTag;
	Refresh();
}

void UKzInputKeysWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Here so the designer shows them too
	for (const auto& [Icon, FaceKey] : GetFaceIcons())
	{
		if (Icon)
		{
			Icon->SetSource(MakeKeySource(FaceKey));
		}
	}
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
	FKey Key;
	FKey ChordKey;
	const UKzUIInputSubsystem* Subsystem = BoundSubsystem.Get();
	const ULocalPlayer* Player = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* EnhancedInput = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player);
	const UEnhancedPlayerInput* PlayerInput = EnhancedInput ? EnhancedInput->GetPlayerInput() : nullptr;
	if (Subsystem && PlayerInput)
	{
		FKzUIIconSource_InputTag Source;
		Source.InputTag = InputTag;
		FKzUIIconContext Context;
		Context.Player = Player;

		// Pad keys also on the keyboard when the game shows pad icons there, as KzInputIcon does
		const bool bGamepad = Subsystem->GetCurrentInputDevice() != EKzUIInputDevice::Keyboard || UKzUIInputSettings::Get()->bGamepadIconsOnKeyboard;
		if (const FEnhancedActionKeyMapping* Mapping = FindMapping(*PlayerInput, Source.GetInputAction(Context), bGamepad))
		{
			Key = Mapping->Key;
			const FEnhancedActionKeyMapping* ChordMapping = FindMapping(*PlayerInput, FindChordAction(*Mapping), bGamepad);
			ChordKey = ChordMapping ? ChordMapping->Key : FKey();
		}
	}

	const TArray<TPair<UKzInputIcon*, FKey>, TInlineAllocator<4>> FaceIcons = GetFaceIcons();
	const bool bFace = FaceButtons && Algo::AnyOf(FaceIcons, [&Key](const TPair<UKzInputIcon*, FKey>& Face) { return Face.Value == Key; });

	SetShown(Chord ? Chord.Get() : ChordIcon.Get(), ChordKey.IsValid());
	if (ChordIcon && ChordKey.IsValid())
	{
		ChordIcon->SetSource(MakeKeySource(ChordKey));
	}

	SetShown(KeyIcon.Get(), Key.IsValid() && !bFace);
	if (KeyIcon && Key.IsValid() && !bFace)
	{
		KeyIcon->SetSource(MakeKeySource(Key));
	}

	SetShown(FaceButtons, bFace);
	for (const auto& [Icon, FaceKey] : FaceIcons)
	{
		if (Icon)
		{
			Icon->SetRenderOpacity(FaceKey == Key ? 1.0f : DimmedOpacity);
		}
	}
}

TArray<TPair<UKzInputIcon*, FKey>, TInlineAllocator<4>> UKzInputKeysWidget::GetFaceIcons() const
{
	return {
		MakeTuple(FaceTop.Get(), EKeys::Gamepad_FaceButton_Top),
		MakeTuple(FaceLeft.Get(), EKeys::Gamepad_FaceButton_Left),
		MakeTuple(FaceRight.Get(), EKeys::Gamepad_FaceButton_Right),
		MakeTuple(FaceBottom.Get(), EKeys::Gamepad_FaceButton_Bottom),
	};
}
