// Copyright 2026 kirzo

#include "Animation/KzAnimationPreview.h"
#include "Animation/KzAnimationSource_LimbicAction.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "LimbicAnimNode.h"
#include "LimbicMarionette.h"
#include "LimbicPreviewInstance.h"
#include "LimbicPuppeteer.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "KzGameplayLimbicEditor"

namespace
{
	/** A Limbic action played on the preview mesh by Limbic's own preview instance, as its puppeteer editor plays it. */
	class FKzLimbicActionPreview : public FKzAnimationPreview
	{
	public:
		FKzLimbicActionPreview(USkeletalMeshComponent& InMesh, ULimbicPreviewInstance& InInstance, const ULimbicPuppeteer& InPuppeteer, float InPlayRate, const FText& InProblem)
			: Mesh(&InMesh), Instance(&InInstance), Puppeteer(&InPuppeteer), PlayRate(InPlayRate), Problem(InProblem)
		{
			SetPlaying(true);
		}

		virtual ~FKzLimbicActionPreview() override
		{
			// Pausing slows the mesh's animation down to nothing; whatever plays next starts at its own pace.
			if (Mesh.IsValid())
			{
				Mesh->GlobalAnimRateScale = 1.0f;
			}
		}

		virtual float GetLength() const override
		{
			return Puppeteer.IsValid() ? Puppeteer->Duration : 0.0f;
		}

		virtual float GetTime() const override
		{
			return Instance.IsValid() ? static_cast<float>(Instance->GetNode().GetPuppeteerInstance().GetTimelineTime()) : 0.0f;
		}

		virtual void SetTime(float Time) override
		{
			const float Length = GetLength();
			if (Instance.IsValid())
			{
				Instance->SetTime(Length > 0.0f ? FMath::Min(Time, Length - UE_KINDA_SMALL_NUMBER) : Time);
			}
		}

		virtual bool IsPlaying() const override
		{
			return bPlaying;
		}

		virtual void SetPlaying(bool bInPlaying) override
		{
			bPlaying = bInPlaying;
			if (Mesh.IsValid())
			{
				Mesh->GlobalAnimRateScale = bPlaying ? PlayRate : 0.0f;
			}
		}

		virtual void Tick(float DeltaTime) override
		{
			// An action holds at its end; the preview plays it again.
			if (bPlaying && Instance.IsValid() && Instance->GetNode().GetPuppeteerInstance().IsTimelineOver())
			{
				Instance->SetTime(0.0);
			}
		}

		virtual FText GetProblem() const override
		{
			return Problem;
		}

	private:
		TWeakObjectPtr<USkeletalMeshComponent> Mesh;
		TWeakObjectPtr<ULimbicPreviewInstance> Instance;
		TWeakObjectPtr<const ULimbicPuppeteer> Puppeteer;
		float PlayRate = 1.0f;
		FText Problem;
		bool bPlaying = true;
	};

	/** The marionette a puppeteer previews on, as its editor finds it: its own, or the project's only one. */
	ULimbicMarionette* FindMarionette(const ULimbicPuppeteer& Puppeteer)
	{
		if (Puppeteer.PreviewMarionette)
		{
			return Puppeteer.PreviewMarionette;
		}

		TArray<FAssetData> Marionettes;
		IAssetRegistry::GetChecked().GetAssetsByClass(ULimbicMarionette::StaticClass()->GetClassPathName(), Marionettes);
		return Marionettes.Num() == 1 ? Cast<ULimbicMarionette>(Marionettes[0].GetAsset()) : nullptr;
	}

	TSharedPtr<FKzAnimationPreview> MakeLimbicActionPreview(const UKzAnimationSource& Source, const FKzAnimationContext& Context, USkeletalMeshComponent& Mesh)
	{
		const UKzAnimationSource_LimbicActionBase& ActionSource = static_cast<const UKzAnimationSource_LimbicActionBase&>(Source);
		ULimbicPuppeteer* Puppeteer = ActionSource.GetPuppeteer(Context);
		ULimbicMarionette* Marionette = Puppeteer ? FindMarionette(*Puppeteer) : nullptr;
		if (!Marionette)
		{
			return nullptr;
		}

		Mesh.SetAnimInstanceClass(ULimbicPreviewInstance::StaticClass());
		ULimbicPreviewInstance* Instance = Cast<ULimbicPreviewInstance>(Mesh.GetAnimInstance());
		if (!Instance)
		{
			return nullptr;
		}
		Instance->SetPreview(Marionette, Puppeteer);

		FText Problem;
		const USkeletalMesh* MeshAsset = Mesh.GetSkeletalMeshAsset();
		const USkeleton* MeshSkeleton = MeshAsset ? MeshAsset->GetSkeleton() : nullptr;
		if (Marionette->Skeleton && MeshSkeleton && Marionette->Skeleton != MeshSkeleton)
		{
			Problem = FText::Format(LOCTEXT("OtherSkeleton", "{0} previews on {1}, made for {2}, but the preview mesh uses {3}."), FText::FromString(Puppeteer->GetName()), FText::FromString(Marionette->GetName()), FText::FromString(Marionette->Skeleton->GetName()), FText::FromString(MeshSkeleton->GetName()));
		}
		return MakeShared<FKzLimbicActionPreview>(Mesh, *Instance, *Puppeteer, ActionSource.PlayRate, Problem);
	}
}

class FKzGameplayLimbicEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		KzAnimationPreview::Register(UKzAnimationSource_LimbicActionBase::StaticClass(), FKzMakeAnimationPreview::CreateStatic(&MakeLimbicActionPreview));
	}

	virtual void ShutdownModule() override
	{
		KzAnimationPreview::Unregister(UKzAnimationSource_LimbicActionBase::StaticClass());
	}
};

IMPLEMENT_MODULE(FKzGameplayLimbicEditorModule, KzGameplayLimbicEditor)

#undef LOCTEXT_NAMESPACE