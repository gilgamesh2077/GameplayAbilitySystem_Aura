// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class UNiagaraSystem;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FTaggedMontage
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag MontageTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag SocketTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundBase* ImpactSound = nullptr;
};

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};



/**
 * 
 */
class AURA_API ICombatInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	int32 GetAuraCharacterLevel() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FVector GetCombatSocketLocation(const FGameplayTag& CombatSocketTag) const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsPlayer() const;
	
	virtual bool IsAttacking() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UpdateFacingRotationFromLocation(const FVector& Location);
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	UAnimMontage* GetHitReactMontage();
	
	/**
	 * 受击状态由受击技能自己的生命周期驱动：技能激活时传 true，EndAbility 时传 false。
	 * 受击技能是所有敌人共用的蓝图资产，走接口调用可以避免蓝图去 Cast 具体敌人类型。
	 */
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	void SetHitReacting(bool bInHitReacting);
	
	virtual void Die() = 0;
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	bool IsDead() const;
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	AActor* GetAvatar();
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	TArray<FTaggedMontage> GetTaggedMontages() const ;
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	UNiagaraSystem* GetBloodEffect();
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	FTaggedMontage GetTaggedMontageByTag(const FGameplayTag& MontageTag) ;
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	int32  GetMinionsCount();
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	void  SetMinionsCount(int32 Count);
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	void  SetMaster(AActor* InMaster);
};
