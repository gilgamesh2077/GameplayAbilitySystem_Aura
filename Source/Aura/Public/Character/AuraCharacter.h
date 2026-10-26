// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "AuraCharacter.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API AAuraCharacter : public AAuraCharacterBase
{
	GENERATED_BODY()
	
public:
	AAuraCharacter();
	

	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	
	/*Combat Interface*/
	virtual int32 GetAuraCharacterLevel_Implementation() const override;
	/*End Combat Interface*/
	
	bool IsPlayer_Implementation() const;
protected:
	virtual void InitAbilityActorInfo() override;
	
};
