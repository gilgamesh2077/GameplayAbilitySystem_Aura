// See you in the battle

#include "AI/BTTask_MoveAwayFromTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

UBTTask_MoveAwayFromTarget::UBTTask_MoveAwayFromTarget()
{
	NodeName = TEXT("Move Away From Target");
	bNotifyTick = true;
	bCreateNodeInstance = true; // 需要保存 LastGoal / 速度等实例状态

	// 只允许选择 Object 类型黑板键（对应 TargetToFollow）
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveAwayFromTarget, BlackboardKey), AActor::StaticClass());
}

void UBTTask_MoveAwayFromTarget::ComputeRetreatLocation(APawn* Pawn, AActor* Target, FVector& OutLocation) const
{
	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	// 基础方向：从目标指向自己（俯视角忽略 Z）
	FVector BaseDir = PawnLocation - TargetLocation;
	BaseDir.Z = 0.f;
	if (BaseDir.IsNearlyZero())
	{
		BaseDir = Pawn->GetActorForwardVector();
		BaseDir.Z = 0.f;
	}
	BaseDir.Normalize();

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavSys)
	{
		// 没有导航系统：直接用理想点，MoveTo 可能失败但不至于崩
		OutLocation = TargetLocation + BaseDir * SafeDistance;
		return;
	}

	// 沿「远离方向」±90° 扇形找可达点，优先取离目标最远的那个
	const float AngleOffsets[] = { 0.f, 30.f, -30.f, 60.f, -60.f, 90.f, -90.f };
	FVector BestLocation = FVector::ZeroVector;
	float BestDistance = -1.f;

	for (const float Angle : AngleOffsets)
	{
		const FVector Dir = BaseDir.RotateAngleAxis(Angle, FVector::UpVector);
		const FVector Candidate = TargetLocation + Dir * SafeDistance;

		FNavLocation NavLoc;
		if (!NavSys->ProjectPointToNavigation(Candidate, NavLoc, FVector(800.f, 800.f, 800.f)))
		{
			continue;
		}

		const float Dist = FVector::Dist2D(TargetLocation, NavLoc.Location);
		if (Dist > BestDistance)
		{
			BestDistance = Dist;
			BestLocation = NavLoc.Location;
			if (Dist >= SafeDistance - AcceptanceRadius)
			{
				break; // 已经够远，直接用
			}
		}
	}

	if (BestDistance < 0.f)
	{
		// 附近没有任何导航点，退回理想点，让 MoveTo 自己找路
		OutLocation = TargetLocation + BaseDir * SafeDistance;
		return;
	}

	OutLocation = BestLocation;
}

bool UBTTask_MoveAwayFromTarget::IssueRetreat(UBehaviorTreeComponent& OwnerComp, AActor* Target)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !Pawn || !Target)
	{
		return false;
	}

	FVector RetreatLocation;
	ComputeRetreatLocation(Pawn, Target, RetreatLocation);

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		RetreatLocation,
		AcceptanceRadius,
		/*bStopOnOverlap=*/true,
		/*bUsePathfinding=*/true,
		/*bProjectDestinationToNavigation=*/false, // 已在 ComputeRetreatLocation 中投影
		/*bCanStrafe=*/true);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return false;
	}

	LastGoal = RetreatLocation;
	return true;
}

void UBTTask_MoveAwayFromTarget::ApplyRetreatSpeed(APawn* Pawn)
{
	if (RetreatSpeedMultiplier <= 1.f || bSpeedBoosted)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Pawn->FindComponentByClass<UCharacterMovementComponent>();
	if (!Movement)
	{
		return;
	}

	CachedMovement = Movement;
	OriginalMaxWalkSpeed = Movement->MaxWalkSpeed;
	Movement->MaxWalkSpeed = OriginalMaxWalkSpeed * RetreatSpeedMultiplier;
	bSpeedBoosted = true;
}

void UBTTask_MoveAwayFromTarget::RestoreSpeed()
{
	if (bSpeedBoosted && CachedMovement.IsValid())
	{
		CachedMovement->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}
	bSpeedBoosted = false;
	CachedMovement = nullptr;
}

EBTNodeResult::Type UBTTask_MoveAwayFromTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Pawn || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName));
	if (!IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}

	const float Dist2D = FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation());
	if (Dist2D >= KiteTriggerDistance)
	{
		// 距离够远，不需要后撤 → 让行为树走攻击 / 追击分支
		return EBTNodeResult::Failed;
	}

	ApplyRetreatSpeed(Pawn);

	if (!IssueRetreat(OwnerComp, Target))
	{
		RestoreSpeed();
		return EBTNodeResult::Failed;
	}

	LastDistance = Dist2D;
	NoProgressTime = 0.f;
	return EBTNodeResult::InProgress;
}

void UBTTask_MoveAwayFromTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Pawn || !Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName));
	if (!IsValid(Target))
	{
		// 目标消失，后撤结束
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const float Dist2D = FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation());

	// 1) 已退到安全距离
	if (Dist2D >= SafeDistance - AcceptanceRadius)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 2) 无进展保护：玩家更快 / 被地形卡住时别无限后撤
	if (Dist2D <= LastDistance + 1.f)
	{
		NoProgressTime += DeltaSeconds;
	}
	else
	{
		NoProgressTime = 0.f;
	}
	LastDistance = Dist2D;

	if (NoProgressTime >= MaxNoProgressTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 3) 目标移动或自己停下后重新规划，避免追着过期目标点
	FVector IdealLocation;
	ComputeRetreatLocation(Pawn, Target, IdealLocation);

	const bool bStopped = AIController->GetMoveStatus() == EPathFollowingStatus::Idle;
	const bool bGoalStale = FVector::DistSquared2D(IdealLocation, LastGoal) > FMath::Square(ReplanThreshold);

	if (bStopped || bGoalStale)
	{
		if (!IssueRetreat(OwnerComp, Target))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}

void UBTTask_MoveAwayFromTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// 无论成功 / 失败 / 被行为树打断，都恢复速度
	RestoreSpeed();
	NoProgressTime = 0.f;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
