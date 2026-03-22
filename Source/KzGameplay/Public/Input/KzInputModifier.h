// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KzInputModifier.generated.h"

class UKzInputModifier;

/** A container for a collection of input modifiers. */
USTRUCT(BlueprintType)
struct FKzInputModifierContainer
{
	GENERATED_BODY()

protected:
	/** Array of modifiers instantiated directly in the details panel or added via code. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Modifiers")
	TArray<TObjectPtr<UKzInputModifier>> Modifiers;

public:
	/** Default constructor */
	FKzInputModifierContainer() = default;

	/** Single modifier constructor */
	FKzInputModifierContainer(UKzInputModifier* InModifier)
	{
		if (InModifier)
		{
			Modifiers.Add(InModifier);
		}
	}

	/** Initializer list constructor for quick inline setup */
	FKzInputModifierContainer(std::initializer_list<UKzInputModifier*> InitList)
	{
		Modifiers.Reserve(InitList.size());
		for (UKzInputModifier* Mod : InitList)
		{
			if (Mod)
			{
				Modifiers.Add(Mod);
			}
		}
	}

	/** TArrayView constructor for safe array passing */
	FKzInputModifierContainer(const TArrayView<UKzInputModifier*> ArrayView)
	{
		Modifiers.Reserve(ArrayView.Num());
		for (UKzInputModifier* Mod : ArrayView)
		{
			if (Mod)
			{
				Modifiers.Add(Mod);
			}
		}
	}

	// --- Iterators ---

	FORCEINLINE auto begin() { return Modifiers.begin(); }
	FORCEINLINE auto end() { return Modifiers.end(); }

	FORCEINLINE auto begin() const { return Modifiers.begin(); }
	FORCEINLINE auto end() const { return Modifiers.end(); }
};

/**
 * Base class for any object that modifies an analog input vector (Movement, Look, etc.).
 * Designed to be stacked.
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType, CollapseCategories)
class KZGAMEPLAY_API UKzInputModifier : public UObject
{
	GENERATED_BODY()

public:
	/** Returns the World context (helper for Blueprint modifiers). */
	virtual UWorld* GetWorld() const override
	{
		if (IsTemplate()) return nullptr;
		return GetOuter() ? GetOuter()->GetWorld() : nullptr;
	}

	/**
	 * Calculates the modified input.
	 * @param Avatar The actor generating the input.
	 * @param OriginalInput The raw input from the controller (before any modifier in the stack).
	 * @param CurrentInput The input as modified by previous modifiers in the stack.
	 * @return The new input vector.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Input Modifier")
	FVector ModifyInput(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const;

private:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const
	{
		return CurrentInput;
	}
};