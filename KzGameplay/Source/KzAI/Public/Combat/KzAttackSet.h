// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KzAttackSet.generated.h"

/** One attack an AI can be given by its combat: when it can, what it costs, and how its tree makes it. */
USTRUCT(BlueprintType)
struct KZAI_API FKzAttackDefinition
{
	GENERATED_BODY()

	/** The tickets it takes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (Categories = "Ticket"))
	FGameplayTag TicketType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "1"))
	int32 Cost = 1;

	/** How likely it is against the other attacks the attacker could make. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0"))
	float Probability = 1.0f;

	/** The areas of the target's engagement it can be made from, by their tag. None allows any area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection")
	FGameplayTagContainer Areas;

	/** Also from beyond the last area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection")
	bool bFromOutside = false;

	/** How long until it can be made again once it is over. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0", Units = "Seconds"))
	float Cooldown = 0.0f;

	/** The same when it was cut short or never landed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0", Units = "Seconds"))
	float CancelledCooldown = 0.0f;

	/** Can't be given until something unblocks it, such as a phase of the fight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection")
	bool bBlocked = false;

	/** Pressed as Press Input does: the first at once, each next one as the attacker starts matching Next When. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (Categories = "Input"))
	TArray<FGameplayTag> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution")
	FGameplayTagQuery NextWhen;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution")
	bool bLoop = false;

	/** How close to the target it is made from; it may lie inside the forbidden area of its engagement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution", meta = (ClampMin = "0", Units = "Centimeters"))
	float Distance = 100.0f;
};

/** The attacks an AI can be given by its combat, by their tag, such as one per kind of enemy. */
UCLASS(BlueprintType)
class KZAI_API UKzAttackSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attacks")
	TMap<FGameplayTag, FKzAttackDefinition> Attacks;
};