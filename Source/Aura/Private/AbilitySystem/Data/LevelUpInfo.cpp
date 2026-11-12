// See you in the battle


#include "AbilitySystem/Data/LevelUpInfo.h"

int32 ULevelUpInfo::FindLevelForXP(TArray<FAuraLevelUpInfo> const &LevelUpInfo,const int32 XP)
{
	int32 r = LevelUpInfo.Num() - 1;
	int32 l = 1;
	while (l<r)
	{
		//LevelUpInformation[i] refer to Level i
		int32 mid = l + (r - l)/2;
		if (LevelUpInfo[mid].LevelUpRequirement > XP) r = mid;
		else l = mid + 1; 
	}
	return l;
}
