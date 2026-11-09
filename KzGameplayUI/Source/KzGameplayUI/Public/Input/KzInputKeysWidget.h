// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "KzUITypes.h"
#include "KzInputKeysWidget.generated.h"

class UKzInputIcon;
class UKzUIInputSubsystem;
class UPanelWidget;

UENUM(BlueprintType)
enum class EKzFaceButtonStyle : uint8
{
	/** Each face button alone, like any other key. */
	Icon,
	/** The face buttons lit together in the face-button diamond. */
	Diamond
};

/** A group of keys in a KzInputKeysWidget row: one key in Icon, or face buttons lit in the face-button diamond, the others dimmed. */
UCLASS(Abstract)
class KZGAMEPLAYUI_API UKzInputKeyGroupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetKeys(const TArray<FKey>& Keys);

protected:
	/** Render opacity of the face buttons that aren't lit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (ClampMin = "0", ClampMax = "1"))
	float DimmedOpacity = 0.35f;

	/** Shows the first key. */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> Icon;

	/** The face-button diamond, whose keys are set here. */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceTop;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceLeft;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceRight;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceBottom;

	virtual void NativePreConstruct() override;

private:
	TArray<TPair<UKzInputIcon*, FKey>, TInlineAllocator<4>> GetFaceIcons() const;
};

/**
 * A row with the keys that fire an input tag's action for the owning player now: the keys its chords need held, then its own key,
 * with a separator between groups. Each key is a Key Class group; with the Diamond style, the face buttons go together in one
 * Diamond Class group. Rebuilt when the keys change with the player's device or key mappings.
 */
UCLASS(Abstract)
class KZGAMEPLAYUI_API UKzInputKeysWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputTag(UPARAM(meta = (Categories = "Input")) FGameplayTag NewInputTag);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	EKzFaceButtonStyle FaceButtonStyle = EKzFaceButtonStyle::Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSubclassOf<UKzInputKeyGroupWidget> KeyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (EditCondition = "FaceButtonStyle == EKzFaceButtonStyle::Diamond"))
	TSubclassOf<UKzInputKeyGroupWidget> DiamondClass;

	/** Goes between groups, such as a "+". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSubclassOf<UUserWidget> SeparatorClass;

#if WITH_EDITORONLY_DATA
	/** Shown in the designer, where there is no player to read the keys from. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<FKey> PreviewKeys = { EKeys::Gamepad_LeftTrigger, EKeys::Gamepad_FaceButton_Bottom };
#endif

	/** Filled with the row, replacing whatever it holds in the designer. */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidget))
	TObjectPtr<UPanelWidget> Keys;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnInputDeviceChanged(EKzUIInputDevice InputDevice) { Refresh(); }

	UFUNCTION()
	void Refresh();

	void ShowKeys(const TArray<FKey>& NewKeys);

	TArray<FKey> ShownKeys;
	TWeakObjectPtr<UKzUIInputSubsystem> BoundSubsystem;
};
