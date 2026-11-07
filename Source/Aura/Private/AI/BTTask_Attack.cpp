// See you in the battle


#include "AI/BTTask_Attack.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "AuraGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/EnemyInterface.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bCreateNodeInstance = true; // 每个行为树各持有一份实例，委托绑定互不干扰
}

void UBTTask_Attack::SetCombatTarget(UBehaviorTreeComponent& OwnerComp, APawn* Pawn)
{
	// 把黑板里的目标设成战斗目标，供 GA_MeleeAttack 的 GetCombatTarget 获取
	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		if (AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(FName("TargetToFollow"))))
		{
			if (Pawn->Implements<UEnemyInterface>())
			{
				IEnemyInterface::Execute_SetCombatTarget(Pawn, Target);
			}
		}
	}
}

void UBTTask_Attack::StopMovement(APawn* Pawn)
{
	if (UCharacterMovementComponent* MovementComp = Pawn->FindComponentByClass<UCharacterMovementComponent>())
	{
		MovementComp->StopMovementImmediately();
	}
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	// 攻击出手前先停下：攻击蒙太奇没有前向根运动、运动扭曲也只改朝向，
	// 若不主动停住，敌人追击（MoveTo）的残留速度会在攻击期间继续把它带向玩家。
	StopMovement(Pawn);
	SetCombatTarget(OwnerComp, Pawn);

	UAbilitySystemComponent* ASC = nullptr;
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
	{
		ASC = ASI->GetAbilitySystemComponent();
	}

	// 默认激活攻击；元素使在 minion 未满阈值时改为召唤
	FGameplayTag AbilityTagToActivate = AttackAbilityTag;
	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		if (Blackboard->GetValueAsBool(FName("ElementalistAttacker")))
		{
			int32 MinionsCount = 0;
			if (Pawn->Implements<UCombatInterface>())
			{
				MinionsCount = ICombatInterface::Execute_GetMinionsCount(Pawn);
			}

			if (MinionsCount < MinionsThreshold && SummonAbilityTag.IsValid() && !ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Cooldown_Summon))
			{
				AbilityTagToActivate = SummonAbilityTag;
			}
		}
	}

	if (!ASC || !AbilityTagToActivate.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// 先绑定、再激活：同步技能（如召唤，无蒙太奇/延迟）会在激活瞬间就结束，
	// 绑定必须提前，否则永远收不到技能结束事件。
	CachedOwnerComp = &OwnerComp;
	CachedASC = ASC;
	ActiveAbilityTag = AbilityTagToActivate;
	AbilityEndedHandle = ASC->AbilityEndedCallbacks.AddUObject(this, &UBTTask_Attack::OnAttackAbilityEnded);

	// 激活技能；按 Tag 激活（与具体类解耦，蓝图子类也能命中）
	const bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTagToActivate));

	if (!bActivated)
	{
		UnbindAbilityEnded();
		return EBTNodeResult::Failed;
	}
	
	return EBTNodeResult::InProgress;
}

void UBTTask_Attack::OnAttackAbilityEnded(UGameplayAbility* Ability)
{
	// 只响应本次激活技能（攻击或召唤）的结束，别的技能（如受击 HitReact）结束不触发
	if (Ability && ActiveAbilityTag.IsValid() && !Ability->AbilityTags.HasTag(ActiveAbilityTag))
	{
		return;
	}
	
	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	UnbindAbilityEnded();
	if (OwnerComp){
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_Attack::UnbindAbilityEnded()
{
	if (AbilityEndedHandle.IsValid() && CachedASC.IsValid())
	{
		CachedASC->AbilityEndedCallbacks.Remove(AbilityEndedHandle);
	}
	AbilityEndedHandle.Reset();
	ActiveAbilityTag = FGameplayTag();
	CachedASC = nullptr;
	CachedOwnerComp = nullptr;
}

void UBTTask_Attack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// 无论正常结束还是被打断，都清理委托绑定
	UnbindAbilityEnded();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
