// Copyright 2026 kirzo

#include "Input/KzInputKeysWidget.h"
#include "Input/KzUIIconSource_InputTag.h"
#include "KzInputIcon.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
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

	bool IsFaceButton(const FKey& Key)
	{
		return Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Gamepad_FaceButton_Left
			|| Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_FaceButton_Bottom;
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
			// The player's mappings also carry the action's own chord triggers. Blockers are the ones Enhanced Input adds to the mappings a chord masks
			for (const UInputTrigger* Trigger : Mapping->Triggers)
			{
				const UInputTriggerChordAction* Chord = Cast<UInputTriggerChordAction>(Trigger);
				const FEnhancedActionKeyMapping* ChordMapping = Chord && !Chord->IsA<UInputTriggerChordBlocker>() ? FindMapping(*PlayerInput, Chord->ChordAction, bGamepad) : nullptr;
				if (ChordMapping)
				{
					NewKeys.Add(ChordMapping->Key);
				}
			}
			NewKeys.Add(Mapping->Key);
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
