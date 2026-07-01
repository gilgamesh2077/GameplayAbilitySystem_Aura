// See you in the battle


#include "Input/AuraInputConfig.h"


const UInputAction* UAuraInputConfig::FindAbilityInputActionForTag(const FGameplayTag InputTag, bool bLogNotFound) const
{
	for (const auto & AuraInputAction : AbilityInputAction)
	{
		if (AuraInputAction.InputAction && InputTag == AuraInputAction.InputTag)
		{
			return AuraInputAction.InputAction;	
		}
	}
	if (bLogNotFound)
	{
		UE_LOG(LogTemp,Error,TEXT("Can't find AbilityInputAction for InputTag [%s}, on InputConfig [%s]"),*InputTag.ToString(),*GetNameSafe(this));
	}
	return nullptr;
}
