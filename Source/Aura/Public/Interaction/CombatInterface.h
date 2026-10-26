// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FTaggedMontage
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag MontageTag;
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
	FVector GetCombatSocketLocation(const FGameplayTag& MontageTag) const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsPlayer() const;
	
	virtual bool IsAttacking() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UpdateFacingRotationFromLocation(const FVector& Location);
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	UAnimMontage* GetHitReactMontage();
	
	virtual void Die() = 0;
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	bool IsDead() const;
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	AActor* GetAvatar();
	
	UFUNCTION(BlueprintNativeEvent , BlueprintCallable)
	TArray<FTaggedMontage> GetTaggedMontages() const ;
};
