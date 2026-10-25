// Copyright 2026 kirzo

#include "Animation/KzAnimationPreview.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/KzAnimationSource.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	TMap<const UClass*, FKzMakeAnimationPreview> Makers;

	class FKzMontagePreview : public FKzAnimationPreview
	{
	public:
		FKzMontagePreview(UAnimSingleNodeInstance& InInstance, UAnimMontage& InMontage) : Instance(&InInstance), Montage(&InMontage) {}

		virtual float GetLength() const override
		{
			return Montage.IsValid() ? Montage->GetPlayLength() : 0.0f;
		}

		virtual float GetTime() const override
		{
			return Instance.IsValid() ? Instance->GetCurrentTime() : 0.0f;
		}

		virtual void SetTime(float Time) override
		{
			if (Instance.IsValid() && Montage.IsValid())
			{
				// A montage that ran out has no instance left to pose.
				if (!Instance->GetActiveMontageInstance())
				{
					Instance->RestartMontage(Montage.Get());
				}
				Instance->SetPosition(Time, false);
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

	private:
		TWeakObjectPtr<UAnimSingleNodeInstance> Instance;
		TWeakObjectPtr<UAnimMontage> Montage;
	};

	TSharedPtr<FKzAnimationPreview> MakeMontagePreview(const UKzAnimationSource& Source, const FKzAnimationContext& Context, USkeletalMeshComponent& Mesh)
	{
		const UKzAnimationSource_MontageBase& MontageSource = static_cast<const UKzAnimationSource_MontageBase&>(Source);
		UAnimMontage* Montage = MontageSource.GetMontage(Context);
		if (!Montage)
		{
			return nullptr;
		}

		Mesh.PlayAnimation(Montage, true);
		UAnimSingleNodeInstance* Instance = Mesh.GetSingleNodeInstance();
		if (!Instance)
		{
			return nullptr;
		}

		Instance->SetPlayRate(MontageSource.PlayRate);
		return MakeShared<FKzMontagePreview>(*Instance, *Montage);
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