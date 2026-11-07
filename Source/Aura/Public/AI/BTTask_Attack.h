// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "Delegates/Delegate.h"
#include "GameplayTagContainer.h"
#include "BTTask_Attack.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * 攻击任务：激活攻击技能，并等待技能结束才返回成功，
 * 避免行为树在技能未播完时反复重入攻击节点。
 */
UCLASS()
class AURA_API UBTTask_Attack : public UBTTask_BlueprintBase
{
	GENERATED_BODY()

public:
	UBTTask_Attack();
	void SetCombatTarget(UBehaviorTreeComponent& OwnerComp, APawn* Pawn);
	void StopMovement(APawn* Pawn);

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// 要激活的攻击技能标签（匹配技能 AbilityTags，需给攻击 GA 打上 Abilities.Attack）
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag AttackAbilityTag;

	// 元素使优先召唤的技能标签（需给召唤 GA 打上 Abilities.Summon）
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag SummonAbilityTag;

	// 元素使 minion 数量阈值：低于此值时召唤，达到后改回攻击
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "1"))
	int32 MinionsThreshold = 5;

private:
	void OnAttackAbilityEnded(UGameplayAbility* Ability);
	void UnbindAbilityEnded();

	// 本次实际激活的技能标签，供结束回调过滤（召唤/攻击二选一）
	FGameplayTag ActiveAbilityTag;
	
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FDelegateHandle AbilityEndedHandle;
};
