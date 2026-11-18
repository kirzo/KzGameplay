// Copyright 2026 kirzo

#include "KzLightingProfile.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "KzLightingProfile"

const FInstancedStruct* UKzLightingProfile::FindModule(const UScriptStruct* Struct) const
{
	return Modules.FindByPredicate([Struct](const FInstancedStruct& Module) { return Module.GetScriptStruct() == Struct; });
}

#if WITH_EDITOR
EDataValidationResult UKzLightingProfile::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TSet<const UScriptStruct*> Kinds;
	for (const FInstancedStruct& Module : Modules)
	{
		const UScriptStruct* Struct = Module.GetScriptStruct();
		if (!Struct) continue;

		bool bAlreadyThere = false;
		Kinds.Add(Struct, &bAlreadyThere);
		if (bAlreadyThere)
		{
			Context.AddError(FText::Format(LOCTEXT("RepeatedModule", "{0} appears more than once; only the first counts."), Struct->GetDisplayNameText()));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE