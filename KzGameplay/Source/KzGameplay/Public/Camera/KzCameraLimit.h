// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "KzCameraLimit.generated.h"

class UBillboardComponent;
class UKzShapeComponent;

UENUM(BlueprintType)
enum class EKzCameraLimitMode : uint8
{
	/** The focus can't get in. */
	Block,

	/** While the target is inside, the focus can't get out. */
	Confine
};

/**
 * A shape that limits where the focus of the camera rigs with a Limits module can go.
 * A blocking limit keeps the focus out; a confining one keeps it in while the target is inside, so its shape can be the room itself.
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZGAMEPLAY_API AKzCameraLimit : public AActor
{
	GENERATED_BODY()

public:
	AKzCameraLimit();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Limit")
	EKzCameraLimitMode Mode = EKzCameraLimitMode::Block;

	/** How far from the surface the focus stays: outside it when blocking, inside it when confining. E.g. half the screen, so the edge stays out of view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Limit", meta = (ClampMin = "0", Units = "Centimeters"))
	float Margin = 0.0f;

	/** After the settings, so they show above the shape's categories. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Limit")
	TObjectPtr<UKzShapeComponent> ShapeComponent;

	/** The shape at its world scale. With the margin, grown by it when blocking and shrunk by it when confining. */
	FKzShapeInstance GetShape(bool bWithMargin) const;

	/** World box around the shape with its margin, from the component's cached bounds. */
	FBox GetBounds() const;

	/** Whether it holds the focus of a target at Location: only a confining limit, with the target inside it. */
	bool IsConfining(const FVector& Location) const;

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
#endif
};