// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

class UKzAnimationSource;
class USkeletalMeshComponent;
struct FKzAnimationContext;

/** An animation source shown on a preview mesh, posed by an editor's timeline. */
class KZGAMEPLAYEDITOR_API FKzAnimationPreview
{
public:
	virtual ~FKzAnimationPreview() = default;

	virtual float GetLength() const = 0;
	virtual float GetTime() const = 0;
	virtual void SetTime(float Time) = 0;
	virtual bool IsPlaying() const = 0;
	virtual void SetPlaying(bool bPlaying) = 0;
};

DECLARE_DELEGATE_RetVal_ThreeParams(TSharedPtr<FKzAnimationPreview>, FKzMakeAnimationPreview, const UKzAnimationSource& /*Source*/, const FKzAnimationContext& /*Context*/, USkeletalMeshComponent& /*Mesh*/);

/** Previews by kind of animation source, registered by editor modules: each kind has its own way to play without an AnimBlueprint. */
namespace KzAnimationPreview
{
	KZGAMEPLAYEDITOR_API void Register(const UClass* SourceClass, const FKzMakeAnimationPreview& Maker);
	KZGAMEPLAYEDITOR_API void Unregister(const UClass* SourceClass);

	/** The preview of the closest registered class the source is; null when none plays it. */
	KZGAMEPLAYEDITOR_API TSharedPtr<FKzAnimationPreview> Make(const UKzAnimationSource& Source, const FKzAnimationContext& Context, USkeletalMeshComponent& Mesh);

	/** Registers the montage preview, which plays a montage in the mesh's single node instance. */
	void RegisterMontagePreview();
}