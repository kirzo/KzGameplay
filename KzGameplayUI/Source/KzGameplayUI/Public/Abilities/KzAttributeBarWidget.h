// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "KzAttributeBarWidget.generated.h"

class APawn;
class UAbilitySystemComponent;
class UProgressBar;
struct FOnAttributeChangeData;

/**
 * A bar for a gameplay attribute of the owning player's pawn, such as its health: Bar shows Attribute as a fraction of MaxAttribute,
 * and follows the player to every pawn they possess.
 */
UCLASS(Abstract)
class KZGAMEPLAYUI_API UKzAttributeBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	FGameplayAttribute Attribute;

	/** The attribute's maximum, which Bar is a fraction of. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	FGameplayAttribute MaxAttribute;

	/** Filled to Attribute over MaxAttribute. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar;

	/** Attribute or its maximum changed, or the bar took a new pawn's values. OldValue is Attribute before the change, the same on a new pawn. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attribute")
	void OnAttributeChanged(float Value, float MaxValue, float OldValue);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

	void Bind(APawn* Pawn);
	void Unbind();
	void Refresh();

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	FDelegateHandle ValueHandle;
	FDelegateHandle MaxHandle;
	float LastValue = 0.0f;
};
