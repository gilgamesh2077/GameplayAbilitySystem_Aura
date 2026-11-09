// See you in the battle

#include "AbilitySystem/Ability/AuraProjectileSpell.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/TargetDataUnderMouse.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/EnemyInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/AuraPlayerController.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"

UAuraProjectileSpell::UAuraProjectileSpell()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	SetAssetTags(FGameplayTagContainer(FAuraGameplayTags::Get().Combo));
	ComboSections = {
		{ TEXT("Attack1") },
		{ TEXT("Attack2") },
		{ TEXT("Attack3") }
	};
	MontageEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.FireBolt"), false);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageAsset(TEXT("/Game/Phoebe/Animation/AM_Phoebe_Attack.AM_Phoebe_Attack"));
	if (AttackMontageAsset.Succeeded())
	{
		AttackMontage = AttackMontageAsset.Object;
	}
}

void UAuraProjectileSpell::RequestTargetData()
{
	if (bWaitingForTargetData)return;

	bWaitingForTargetData = true;
	TargetDataUnderMouseTask = UTargetDataUnderMouse::CreateTargetDataUnderMouse(this);
	TargetDataUnderMouseTask->ValidData.AddDynamic(this,&ThisClass::UAuraProjectileSpell::HandleTargetDataUnderMouse); 
	TargetDataUnderMouseTask->ReadyForActivation();
}

void UAuraProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	// 敌人（AI）：完全交给蓝图的 Event ActivateAbility 驱动（播蒙太奇 → 蓝图通知 → 生成投射物）
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->Implements<UEnemyInterface>())
	{
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		return;
	}

	// —— 以下为玩家逻辑 ——
	// 连击属于同一次技能：起手只检查并扣除消耗。冷却由整段连击共享，
	// 在连击真正结束（FinishComboChain）时才提交，避免冷却从 Attack1 起手就被消耗掉。
	// 冷却期间的拦截仍由激活阶段的 CanActivateAbility → CheckCooldown 完成。
	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	USkeletalMeshComponent* Mesh = GetAvatarActorFromActorInfo()
		? GetAvatarActorFromActorInfo()->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;
	if (!AttackMontage || ComboSections.IsEmpty() || !Mesh || !Mesh->GetAnimInstance())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentComboSectionIndex = 0;
	AAuraPlayerController* APC = Cast<AAuraPlayerController>(ActorInfo->PlayerController);
	if (APC)
	{
		APC->bAttacking = true;
	}
	if (UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->ResetComboAttackWindow();
	}
	
	RequestTargetData();
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, AttackPlayRate, ComboSections[CurrentComboSectionIndex].SectionName, true, RootMotionTranslationScale);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
	
	WaitForComboInput();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		for (const FComboAttackSectionSettings& Section : ComboSections)
		{
			ASC->CurrentMontageSetNextSectionName(
				Section.SectionName,
				NAME_None);
		}
	}

	if (MontageEventTag.IsValid())
	{
		MontageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MontageEventTag, nullptr, false, true);
		MontageEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleMontageEvent);
		MontageEventTask->ReadyForActivation();
	}

	ComboWindowClosedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FAuraGameplayTags::Get().Event_Montage_ComboWindowClosed, nullptr, false, true);
	ComboWindowClosedTask->EventReceived.AddDynamic(this, &ThisClass::HandleComboWindowClosed);
	ComboWindowClosedTask->ReadyForActivation();
}



void UAuraProjectileSpell::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
	AAuraPlayerController* APC = ActorInfo ? Cast<AAuraPlayerController>(ActorInfo->PlayerController) : nullptr;
	if (APC)
	{
		APC->bAttacking = false;
	}
	ResetComboState();
	if (UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->ResetComboAttackWindow();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraProjectileSpell::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	// 移动等取消操作同样要消耗整段连击共享的冷却：能力还处于激活状态时先提交冷却，
	// 再交给父类执行正常的取消 → EndAbility 流程。
	if (IsActive() && CanBeCanceled())
	{
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true);
	}

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}


bool UAuraProjectileSpell::QueueNextComboSection()
{
	if (QueuedComboSectionIndex != INDEX_NONE) return false;
	if (!IsActive() || CurrentComboSectionIndex == INDEX_NONE ) return false;

	USkeletalMeshComponent* Mesh = GetAvatarActorFromActorInfo()
		? GetAvatarActorFromActorInfo()->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance || !IsComboAttackWindowOpen()) return false;

	const FName CurrentSection = ComboSections[CurrentComboSectionIndex].SectionName;
	if (AnimInstance->Montage_GetCurrentSection(AttackMontage) != CurrentSection) return false;

	QueuedComboSectionIndex = CurrentComboSectionIndex + 1;
	if (!ComboSections.IsValidIndex(QueuedComboSectionIndex))
	{
		QueuedComboSectionIndex = INDEX_NONE;
		return false;
	}
	return true;
}

bool UAuraProjectileSpell::IsComboAttackWindowOpen() const
{
	if (const UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		return ASC->IsComboAttackWindowOpen();
	}
	return false;
}

const FComboAttackSectionSettings* UAuraProjectileSpell::GetCurrentComboSectionSettings() const
{
	return ComboSections.IsValidIndex(CurrentComboSectionIndex) ? &ComboSections[CurrentComboSectionIndex] : nullptr;
}


void UAuraProjectileSpell::HandleTargetDataUnderMouse(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	if (DataHandle.IsValid(0))
	{
		const FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(DataHandle, 0);
		if (HitResult.bBlockingHit)
		{
			ProjectileTargetLocation = HitResult.Location;
			ICombatInterface::Execute_UpdateFacingRotationFromLocation(GetAvatarActorFromActorInfo(),ProjectileTargetLocation);
		}
	}

	if (TargetDataUnderMouseTask)
	{
		TargetDataUnderMouseTask->EndTask();
	}
	TargetDataUnderMouseTask = nullptr;
	bWaitingForTargetData = false;
}

void UAuraProjectileSpell::HandleMontageCompleted()
{
	// 蒙太奇自然播完（没有窗口可接续时走窗口关闭分支）：整段连击结束。
	FinishComboChain();
}

void UAuraProjectileSpell::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAuraProjectileSpell::HandleMontageEvent(FGameplayEventData Payload)
{
	// 本段的火球已经出手：从这一刻起才允许用移动打断当前连击。
	// 在此之前移动输入会被锁定，保证挥砍前摇不会被打断。
	if (UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->SetComboAttackInterruptible(true);
	}

	if (ProjectileTargetLocation == FVector()) return;
	SpawnProjectile(ProjectileTargetLocation ,FAuraGameplayTags::Get().CombatSocket_Weapon);
}

void UAuraProjectileSpell::HandleComboWindowClosed(FGameplayEventData Payload)
{
	const int32 NextComboSectionIndex = QueuedComboSectionIndex;
	QueuedComboSectionIndex = INDEX_NONE;

	if (!ComboSections.IsValidIndex(NextComboSectionIndex))
	{
		// 没有接上下一段：不结束能力，也不重播第一段。
		// 让当前段把收招（后摇）播完；因为本段火球已经出手，这段时间里移动输入
		// 可以通过 CancelAbility 打断（并提交共享冷却）。播完后由 OnCompleted 收尾。
		return;
	}

	CurrentComboSectionIndex = NextComboSectionIndex;

	if (UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		// 进入下一段后重新锁定移动，直到该段自己的火球出手。
		ASC->SetComboAttackInterruptible(false);
		ASC->CurrentMontageJumpToSection(ComboSections[CurrentComboSectionIndex].SectionName);
	}
}

void UAuraProjectileSpell::ResetComboState()
{
	bWaitingForTargetData = false;
	CurrentComboSectionIndex = INDEX_NONE;
	QueuedComboSectionIndex = INDEX_NONE;
	ProjectileTargetLocation = FVector();
	if (ComboWindowClosedTask)
	{
		ComboWindowClosedTask->EndTask();
		ComboWindowClosedTask = nullptr;
	}
	if (MontageEventTask)
	{
		MontageEventTask->EndTask();
		MontageEventTask = nullptr;
	}
	if (MontageTask)
	{
		// 不要在这里手动 EndTask：UGameplayTask::EndTask 走的是 OnDestroy(false)，
		// PlayMontageAndWait 不会执行 StopPlayingMontage，蒙太奇会在能力结束后继续播放。
		// 保留给 Super::EndAbility 的 TaskOwnerEnded → OnDestroy(true) 去停蒙太奇。
		MontageTask = nullptr;
	}
	if (TargetDataUnderMouseTask)
	{
		TargetDataUnderMouseTask->EndTask();
		TargetDataUnderMouseTask = nullptr;
	}
	if (ComboInputTask)
	{
		ComboInputTask->EndTask();
		ComboInputTask = nullptr;
	}

}

void UAuraProjectileSpell::WaitForComboInput()
{
	if (!IsActive())
	{
		return;
	}
	
	ComboInputTask =
		UAbilityTask_WaitInputPress::WaitInputPress(this, false);

	ComboInputTask->OnPress.AddDynamic(
		this,
		&ThisClass::HandleComboInputPressed);

	ComboInputTask->ReadyForActivation();
}

void UAuraProjectileSpell::HandleComboInputPressed(float TimeWaited)
{
	ComboInputTask = nullptr;

	// 只有连击窗口内的输入才会排队下一段；窗口外（前摇/后摇）的输入一律忽略。
	// 连段没能接上时由窗口关闭事件直接结束能力，下一次输入会重新激活技能并重新走冷却检查。
	QueueNextComboSection();

	RequestTargetData();

	if (IsActive())
	{
		WaitForComboInput();
	}
}

void UAuraProjectileSpell::FinishComboChain()
{
	if (!IsActive())
	{
		return;
	}

	// 整段连击（Attack1 → Attack2 → Attack3）共用同一份冷却：只在连击真正结束时
	// 提交一次。ForceCooldown = true，跳过 CheckCooldown，避免旧冷却标签让本次提交失败。
	CommitAbilityCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);

	// 结束能力：ResetComboState 会停掉蒙太奇与所有监听任务。
	// 下一次攻击输入会重新 TryActivateAbility，届时才由 CanActivateAbility → CheckCooldown 拦截。
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
