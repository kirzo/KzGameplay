// Copyright 2026 kirzo

#include "Animation/KzAnimationPreview.h"
#include "Animation/AnimMontage.h"
#include "Animation/KzAnimationSource.h"
#include "Animation/KzMontagePreviewInstance.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

#define LOCTEXT_NAMESPACE "KzAnimationPreview"

namespace
{
	TMap<const UClass*, FKzMakeAnimationPreview> Makers;

	class FKzMontagePreview : public FKzAnimationPreview
	{
	public:
		FKzMontagePreview(UKzMontagePreviewInstance& InInstance, UAnimMontage& InMontage, float InPlayRate, const FText& InProblem) : Instance(&InInstance), Montage(&InMontage), PlayRate(InPlayRate), Problem(InProblem)
		{
			SetRate(1.0f);
		}

		virtual float GetLength() const override
		{
			return Montage.IsValid() ? Montage->GetPlayLength() : 0.0f;
		}

		virtual float GetTime() const override
		{
			return Instance.IsValid() ? Instance->GetTime() : 0.0f;
		}

		virtual void SetTime(float Time) override
		{
			if (Instance.IsValid())
			{
				Instance->SetTime(Time);
			}
		}

		virtual bool IsPlaying() const override
		{
			return Instance.IsValid() && Instance->IsPlaying();
		}

		virtual void SetPlaying(bool bPlaying) override
		{
			if (Instance.IsValid())
			{
				Instance->SetPlaying(bPlaying);
			}
		}

		virtual void SetRate(float Rate) override
		{
			if (Instance.IsValid())
			{
				Instance->SetPlayRate(PlayRate * Rate);
			}
		}

		virtual float GetPlayRate() const override
		{
			return PlayRate;
		}

		virtual FText GetProblem() const override
		{
			return Problem;
		}

		virtual UObject* GetAsset() const override
		{
			return Montage.Get();
		}

		virtual float GetBlendInAlpha(float ElapsedTime) const override
		{
			const float BlendTime = Montage.IsValid() ? Montage->BlendIn.GetBlendTime() : 0.0f;
			if (BlendTime <= 0.0f)
			{
				return 1.0f;
			}

			// The blend runs in real time, whatever the play rate.
			const float Alpha = FMath::Clamp(ElapsedTime / (PlayRate * BlendTime), 0.0f, 1.0f);
			return FAlphaBlend::AlphaToBlendOption(Alpha, Montage->BlendIn.GetBlendOption(), Montage->BlendIn.GetCustomCurve());
		}

		virtual void SetBlendFrom(const FKzAnimationPreview* From, float FromTime, float Weight) override
		{
			if (Instance.IsValid())
			{
				Instance->SetBlendFrom(From ? Cast<UAnimMontage>(From->GetAsset()) : nullptr, FromTime, Weight);
			}
		}

	private:
		TWeakObjectPtr<UKzMontagePreviewInstance> Instance;
		TWeakObjectPtr<UAnimMontage> Montage;
		float PlayRate = 1.0f;
		FText Problem;
	};

	TSharedPtr<FKzAnimationPreview> MakeMontagePreview(const UKzAnimationSource& Source, const FKzAnimationContext& Context, USkeletalMeshComponent& Mesh)
	{
		const UKzAnimationSource_MontageBase& MontageSource = static_cast<const UKzAnimationSource_MontageBase&>(Source);
		UAnimMontage* Montage = MontageSource.GetMontage(Context);
		if (!Montage)
		{
			return nullptr;
		}

		Mesh.SetAnimInstanceClass(UKzMontagePreviewInstance::StaticClass());
		UKzMontagePreviewInstance* Instance = Cast<UKzMontagePreviewInstance>(Mesh.GetAnimInstance());
		if (!Instance)
		{
			return nullptr;
		}
		Instance->Play(Montage);

		// On another skeleton only the bones named alike move, and the rest keeps its rest pose.
		FText Problem;
		const USkeleton* AnimationSkeleton = Montage->GetSkeleton();
		const USkeletalMesh* MeshAsset = Mesh.GetSkeletalMeshAsset();
		const USkeleton* MeshSkeleton = MeshAsset ? MeshAsset->GetSkeleton() : nullptr;
		if (AnimationSkeleton && MeshSkeleton && AnimationSkeleton != MeshSkeleton)
		{
			Problem = FText::Format(LOCTEXT("OtherSkeleton", "{0} is made for {1}, but the preview mesh uses {2}."), FText::FromString(Montage->GetName()), FText::FromString(AnimationSkeleton->GetName()), FText::FromString(MeshSkeleton->GetName()));
		}
		return MakeShared<FKzMontagePreview>(*Instance, *Montage, MontageSource.PlayRate, Problem);
	}
}

void KzAnimationPreview::Register(const UClass* SourceClass, const FKzMakeAnimationPreview& Maker)
{
	Makers.Add(SourceClass, Maker);
}

void KzAnimationPreview::Unregister(const UClass* SourceClass)
{
	Makers.Remove(SourceClass);
}

TSharedPtr<FKzAnimationPreview> KzAnimationPreview::Make(const UKzAnimationSource& Source, const FKzAnimationContext& Context, USkeletalMeshComponent& Mesh)
{
	for (const UClass* Class = Source.GetClass(); Class; Class = Class->GetSuperClass())
	{
		if (const FKzMakeAnimationPreview* Maker = Makers.Find(Class))
		{
			return Maker->Execute(Source, Context, Mesh);
		}
	}
	return nullptr;
}

void KzAnimationPreview::RegisterMontagePreview()
{
	Register(UKzAnimationSource_MontageBase::StaticClass(), FKzMakeAnimationPreview::CreateStatic(&MakeMontagePreview));
}

#undef LOCTEXT_NAMESPACE