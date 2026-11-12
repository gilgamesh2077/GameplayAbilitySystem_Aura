// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "AuraPlayerState.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;


DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, int32 /*StateValue*/)

UCLASS()
class AURA_API AAuraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	
	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	AAuraPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
	FOnPlayerStateChanged OnXPChangedDelegate;
	
	FOnPlayerStateChanged OnLevelChangedDelegate;
	
	FORCEINLINE int32 GetPlayerLevel() const {return Level;}
	
	FORCEINLINE void SetPlayerLevel(const int32 InPlayerLevel) {Level = InPlayerLevel; OnLevelChangedDelegate.Broadcast(Level);}
	
	FORCEINLINE void AddToPlayerLevel(const int32 PlayerLevelBonus) {Level += PlayerLevelBonus;OnLevelChangedDelegate.Broadcast(Level);}
	
	FORCEINLINE int32 GetXP() const {return XP;}
	
	FORCEINLINE void SetXP(const int32 InXP) {XP = InXP; OnXPChangedDelegate.Broadcast(XP);}
	
	FORCEINLINE void AddToXP(const int32 XPBonus) {XP += XPBonus;OnXPChangedDelegate.Broadcast(XP);}

protected:
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
private:
	
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = On_RepLevel)
	int32 Level = 1;
	
	UPROPERTY(VisibleAnywhere ,ReplicatedUsing = On_RepXP)
	int32 XP = 0;
	
	UFUNCTION()
	void On_RepLevel(int32 OldLevel) const;
	
	UFUNCTION()
	void On_RepXP(int32 OldXP) const;
	
};
