#include "AuraAbilityTypes.h"

bool FAuraGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint32 RepBits = 0;
	if (Ar.IsSaving())
	{
		if (bIsBlockedHit)
		{
			RepBits |= 1 << 0;
		}
		if (bIsCriticalHit)
		{
			RepBits |= 1 << 1;
		}
	}

	Super::NetSerialize(Ar, Map, bOutSuccess);
	Ar.SerializeBits(&RepBits, 2);

	if (Ar.IsLoading())
	{
		bIsBlockedHit = RepBits & (1 << 0);
		bIsCriticalHit = RepBits & (1 << 1);
	}

	bOutSuccess = true;
	return true;
}
