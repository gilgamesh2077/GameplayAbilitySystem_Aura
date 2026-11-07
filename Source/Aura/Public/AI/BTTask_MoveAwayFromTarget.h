// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveAwayFromTarget.generated.h"

class AAIController;
class APawn;
class UCharacterMovementComponent;

/**
 * 远程敌人后撤 / 放风筝。
 *
 * 相比第一版修的坑：
 *  1. 每帧重算「理想后撤点」：目标会移动，只在开始时算一次会一直追过期点，永远拉不开距离。
 *  2. 后撤点先投影到 NavMesh，并沿「远离方向」±90° 扇形找可达点，避免落在墙后 / 地图外。
 *  3. 无进展保护：玩家更快或被地形卡住时，连续 MaxNoProgressTime 秒拉不开距离就结束（Succeeded），
 *     不会无限 InProgress 卡死。
 *
 * 返回语义（方便在 Selector 里当守卫）：
 *  - Failed    ：距离 >= KiteTriggerDistance，无需后撤，让行为树继续走攻击 / 追击。
 *  - InProgress：正在后撤。
 *  - Succeeded ：已退到 SafeDistance（或判定为无法继续拉开距离）。
 */
UCLASS()
class AURA_API UBTTask_MoveAwayFromTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_MoveAwayFromTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	/** 计算一个「远离目标」的可达点（沿远离方向 ±90° 扇形找导航点）。 */
	void ComputeRetreatLocation(APawn* Pawn, AActor* Target, FVector& OutLocation) const;

	/** 朝后退点发起一次 MoveTo，成功则记录目标点。 */
	bool IssueRetreat(UBehaviorTreeComponent& OwnerComp, AActor* Target);

	/** 后撤期间临时提高移动速度（可选，RetreatSpeedMultiplier > 1 时生效）。 */
	void ApplyRetreatSpeed(APawn* Pawn);
	void RestoreSpeed();

	// 触发后撤的距离阈值：与目标距离小于此值开始后撤
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float KiteTriggerDistance = 100.f;

	// 后撤要达到的安全距离（建议大于 KiteTriggerDistance，形成迟滞区间避免抖动）
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float SafeDistance = 400.f;

	// 到达判定半径
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float AcceptanceRadius = 60.f;

	// 新理想点偏离上次目标超过该距离才重新发起移动（越小越跟手，越大越省路径重算）
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float ReplanThreshold = 100.f;

	// 连续多久（秒）拉不开距离就放弃后撤（防止玩家更快 / 地形卡住时无限后撤）
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float MaxNoProgressTime = 1.f;

	// 后撤时移动速度倍率；1 = 不加速。敌人比玩家慢时可设 1.5~3
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "1"))
	float RetreatSpeedMultiplier = 1.f;

private:
	// 上一次实际发起的后退目标点（用于判断是否过期）
	FVector LastGoal = FVector::ZeroVector;

	// 速度暂存（后撤期间可能临时加速，结束时恢复）
	TWeakObjectPtr<UCharacterMovementComponent> CachedMovement;
	float OriginalMaxWalkSpeed = 0.f;
	bool bSpeedBoosted = false;

	// 无进展检测
	float LastDistance = 0.f;
	float NoProgressTime = 0.f;
};
