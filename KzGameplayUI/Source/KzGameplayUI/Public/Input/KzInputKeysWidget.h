// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "KzUITypes.h"
#include "KzInputKeysWidget.generated.h"

class UKzInputIcon;
class UKzUIInputSubsystem;

/**
 * The keys that fire an input tag's action for the owning player now: the key of a chord the action needs held, then its own key.
 * A face button lights up inside the face-button diamond, the other three dimmed; any other key shows alone in KeyIcon.
 * Refreshed when the player's device or key mappings change.
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

	/** Render opacity of the three face buttons that aren't the key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (ClampMin = "0", ClampMax = "1"))
	float DimmedOpacity = 0.35f;

	/** Visible while the action is chorded. Holds ChordIcon and whatever joins it to the key, such as a "+". */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Chord;

	/** The key held for the chord. */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> ChordIcon;

	/** The key, unless it is a face button and FaceButtons exists. */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> KeyIcon;

	/** Visible instead of KeyIcon when the key is a face button. Holds the four face icons, whose keys are set here. */
	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> FaceButtons;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceTop;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceLeft;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceRight;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (BindWidgetOptional))
	TObjectPtr<UKzInputIcon> FaceBottom;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnInputDeviceChanged(EKzUIInputDevice InputDevice) { Refresh(); }

	UFUNCTION()
	void Refresh();

	/** The face icons, each with its key. */
	TArray<TPair<UKzInputIcon*, FKey>, TInlineAllocator<4>> GetFaceIcons() const;

	TWeakObjectPtr<UKzUIInputSubsystem> BoundSubsystem;
};
