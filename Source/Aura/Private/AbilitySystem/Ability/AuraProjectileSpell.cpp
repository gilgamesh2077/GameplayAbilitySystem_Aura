// See you in the battle

#include "AbilitySystem/Ability/AuraProjectileSpell.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Actor/AuraProjectile.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/TargetDataUnderMouse.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interaction/CombatInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/AuraPlayerController.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"

UAuraProjectileSpell::UAuraProjectileSpell()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	SetAssetTags(FGameplayTagContainer(FAuraGameplayTags::Get().Abilities_Attack_Combo));
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
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
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
			if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
			{
				CombatInterface->UpdateFacingRotationFromLocation(ProjectileTargetLocation);
			}
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
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAuraProjectileSpell::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAuraProjectileSpell::HandleMontageEvent(FGameplayEventData Payload)
{
	if (ProjectileTargetLocation == FVector()) return;
	SpawnProjectile(ProjectileTargetLocation);
}

void UAuraProjectileSpell::HandleComboWindowClosed(FGameplayEventData Payload)
{
	CurrentComboSectionIndex = QueuedComboSectionIndex;
	QueuedComboSectionIndex = INDEX_NONE;

	if (!ComboSections.IsValidIndex(CurrentComboSectionIndex))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC =
		GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CurrentMontageJumpToSection(ComboSections[CurrentComboSectionIndex].SectionName);
	}
}

void UAuraProjectileSpell::SpawnProjectile(const FVector& TargetLocation) const
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass) return;

	if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
	{
		FTransform SpawnTransform;
		const FVector SocketLocation = CombatInterface->GetCombatSocketLocation();
		FRotator Rotation = (TargetLocation - SocketLocation).Rotation();
		Rotation.Pitch = 0.0f;
		
		SpawnTransform.SetRotation(Rotation.Quaternion());
		SpawnTransform.SetLocation(SocketLocation);
		
		AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(ProjectileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			Cast<APawn>(GetOwningActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		Projectile->FinishSpawning(SpawnTransform);
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
		MontageTask->EndTask();
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

	QueueNextComboSection();
	RequestTargetData();

	if (IsActive())
	{
		WaitForComboInput();
	}
}
