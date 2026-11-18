// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/PlayerInterface.h"
#include "AuraCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UNiagaraComponent;
/**
 * 
 */
UCLASS()
class AURA_API AAuraCharacter : public AAuraCharacterBase, public IPlayerInterface
{
	GENERATED_BODY()
	
public:
	AAuraCharacter();
	

	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	
	/*Player Interface*/
	virtual void AddToXP_Implementation(int32 InXP) override;
	
	virtual void LevelUp_Implementation() override;
	
	virtual int32 GetXP_Implementation() const override;
	
	virtual int32 FindLevelForXP_Implementation(const int32 XP) override;
	
	virtual int32 GetAttributePointsReward_Implementation(const int32 Level) override;
	
	virtual int32 GetSpellPointsReward_Implementation(const int32 Level) override;
	
	virtual void AddToPlayerLevel_Implementation(const int32 InLevel) override;
	
	virtual void AddToAttributePoints_Implementation(const int32 InAttributePoints) override;
	
	virtual void AddToSpellPoints_Implementation(const int32 InSpellPoints) override;
	
	virtual int32 GetAttributePoints_Implementation() const override;
	
	virtual int32 GetSpellPoints_Implementation() const override;
	/*Combat Interface*/
	virtual int32 GetAuraCharacterLevel_Implementation() const override;
	/*End Combat Interface*/
	
	virtual bool IsPlayer_Implementation() const override;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> LevelUpNiagaraComponent;
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> TopDownCameraComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	virtual void InitAbilityActorInfo() override;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLevelUpParticles() const;
};
