// See you in the battle


#include "AbilitySystem/Data/AbilityInfo.h"

#include "Aura/AuraLogChannels.h"

FAuraAbilityInfo UAbilityInfo::FindAbilityInfoFromTag(const FGameplayTag& AbilityTag, bool bLogNotFound) const
{
	for (auto Item : AbilityInformation)
	{
		if (Item.AbilityTag == AbilityTag) return Item;
	}
	
	if (bLogNotFound)
	{
		UE_LOG(LogAura, Error, TEXT("Can't find info for AbilityTag [%s] on AbilityInfo [%s]"), *AbilityTag.ToString(), *GetNameSafe(this));
	}
	
	return FAuraAbilityInfo();
}
