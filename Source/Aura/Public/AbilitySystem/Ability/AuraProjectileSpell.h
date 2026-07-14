// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "AuraGameplayAbility.h"
#include "AuraProjectileSpell.generated.h"

class UTargetDataUnderMouse;
class AAuraProjectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimInstance;
class UAnimMontage;
class UAbilityTask_WaitInputPress;

USTRUCT(BlueprintType)
struct FComboAttackSectionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FName SectionName = NAME_None;
};

UCLASS()
class AURA_API UAuraProjectileSpell : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	UAuraProjectileSpell();
	void RequestTargetData();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<AAuraProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<FComboAttackSectionSettings> ComboSections;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.1"))
	float AttackPlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	float RootMotionTranslationScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	FGameplayTag MontageEventTag;
	
	UFUNCTION()
	void HandleTargetDataUnderMouse(const FGameplayAbilityTargetDataHandle& DataHandle);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleMontageEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleComboWindowClosed(FGameplayEventData Payload);

	bool QueueNextComboSection();
	bool IsComboAttackWindowOpen() const;
	const FComboAttackSectionSettings* GetCurrentComboSectionSettings() const;
	void SpawnProjectile(const FVector& TargetLocation) const;
	void ResetComboState();

	UPROPERTY()
	TObjectPtr<UTargetDataUnderMouse> TargetDataUnderMouseTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> MontageEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ComboWindowClosedTask;
	
	void WaitForComboInput();

	UFUNCTION()
	void HandleComboInputPressed(float TimeWaited);

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> ComboInputTask;

	int32 CurrentComboSectionIndex = INDEX_NONE;
	int32 QueuedComboSectionIndex = INDEX_NONE;
	
private:
	FVector ProjectileTargetLocation;
	bool bWaitingForTargetData = false;
};
