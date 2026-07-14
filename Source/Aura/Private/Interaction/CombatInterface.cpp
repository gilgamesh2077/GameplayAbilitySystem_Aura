// See you in the battle


#include "Interaction/CombatInterface.h"


// Add default functionality here for any ICombatInterface functions that are not pure virtual.
int32 ICombatInterface::GetPlayerLevel() const
{
	return 0;
}

FVector ICombatInterface::GetCombatSocketLocation() const
{
	return FVector();
}

bool ICombatInterface::IsAttacking() const
{
	return false;
}

void ICombatInterface::UpdateFacingRotationFromLocation(const FVector& Location)
{
	
}
