// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "ProjectileSpell.h"
#include "AuraProjectileSpell.generated.h"

class UTargetDataUnderMouse;
class AAuraProjectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimInstance;
class UAnimMontage;
class UAbilityTask_WaitInputPress;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FComboAttackSectionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FName SectionName = NAME_None;
};

UCLASS()
class AURA_API UAuraProjectileSpell : public UProjectileSpell
{
	GENERATED_BODY()

public:
	UAuraProjectileSpell();
	void RequestTargetData();

	// 连击窗口结束且没接上下一段时置为 true：冷却已开始，能力可能还在播后摇。
	// ASC 用它决定“再次按键”是排队连击还是结束旧实例重新激活。
	virtual bool IsComboChainFinished() const override { return bComboCooldownCommitted; }

private:
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                           const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;


protected:
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
	void FinishComboChain();
	void CommitComboCooldownOnce();
	bool IsComboAttackWindowOpen() const;
	const FComboAttackSectionSettings* GetCurrentComboSectionSettings() const;
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
	// 整段连击只提交一次冷却：窗口结束没接上 / 被取消 / 蒙太奇播完，先到者生效
	bool bComboCooldownCommitted = false;
	
private:
	FVector ProjectileTargetLocation;
	bool bWaitingForTargetData = false;
};
