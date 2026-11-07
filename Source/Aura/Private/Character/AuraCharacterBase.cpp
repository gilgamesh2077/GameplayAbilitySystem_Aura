// See you in the battle


#include "Character/AuraCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Player/AuraPlayerController.h"
#include "MotionWarpingComponent.h"
#include "Kismet/GameplayStatics.h"

AAuraCharacterBase::AAuraCharacterBase()
{
 	
	PrimaryActorTick.bCanEverTick = false;
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera,ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	MotionWarpingComponent =  CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(),FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}




void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector AAuraCharacterBase::GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag) const
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_Weapon) && IsValid(Weapon))
	{
		return Weapon -> GetSocketLocation(WeaponTipSocketName);
	}
	
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_LeftHand))
	{
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}
	
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_RightHand))
	{
		return GetMesh()-> GetSocketLocation(RightHandSocketName);
	}
	
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_Tail))
	{
		return GetMesh()-> GetSocketLocation(TailSocketName);
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,2.f,FColor::Green,FString::Printf( TEXT("WeaponSocketWrong")));
	}
	return FVector::ZeroVector;
}


bool AAuraCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

AActor* AAuraCharacterBase::GetAvatar_Implementation()
{
	return this;
}

bool AAuraCharacterBase::IsAttacking() const
{
	const AAuraPlayerController* APC =Cast<AAuraPlayerController>(GetController());
	return APC ? APC->bAttacking : false;
}

void AAuraCharacterBase::UpdateAttackWarpTarget(const FVector& TargetLocation)
{
	FVector WarpLocation = TargetLocation;
	WarpLocation.Z = GetActorLocation().Z; // 普通地面攻击通常不扭曲高度

	FVector Direction = WarpLocation - GetActorLocation();
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator FacingRotation = Direction.Rotation();

	MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform(
		TEXT("FacingTarget"),
		FTransform(FacingRotation, WarpLocation));
}

void AAuraCharacterBase::UpdateFacingRotationFromLocation_Implementation(const FVector& Location)
{
	UpdateAttackWarpTarget(Location);
}

UAnimMontage* AAuraCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;	
}

FTaggedMontage AAuraCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag)
{
	for (FTaggedMontage TaggedMontage : AttackMontages)
	{
		if (TaggedMontage.MontageTag == MontageTag) return TaggedMontage;
	}
	return FTaggedMontage();
}

TArray<FTaggedMontage> AAuraCharacterBase::GetTaggedMontages_Implementation() const
{
	return AttackMontages;
}

void AAuraCharacterBase::Die()
{
	if (bDead)
	{
		return; // 已死亡：防止重复处理，也避免重复扣减 master 的随从计数
	}

	// 若自身是被召唤出的随从，死亡时通知 master 更新其拥有的随从数量
	if (Master.IsValid())
	{
		Execute_SetMinionsCount(Master.Get(),FMath::Max(0,Execute_GetMinionsCount(Master.Get()) - 1));
	}

	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld,true));
	MulticastHandleDeath();
}

UNiagaraSystem* AAuraCharacterBase::GetBloodEffect_Implementation()
{
	return BloodEffect;
}

int32 AAuraCharacterBase::GetMinionsCount_Implementation()
{
	return MinionsCount;
}

void AAuraCharacterBase::SetMinionsCount_Implementation(int32 Count)
{
	MinionsCount = Count;
}

void AAuraCharacterBase::SetMaster_Implementation(AActor* InMaster)
{
	Master = Cast<AAuraCharacterBase>(InMaster);
}


void AAuraCharacterBase::MulticastHandleDeath_Implementation()
{
	
	UGameplayStatics::PlaySoundAtLocation(this,DeathSound,GetActorLocation());
	
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Weapon->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
	
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dissolve();
	bDead = true;
}

void AAuraCharacterBase::InitAbilityActorInfo()
{
}

void AAuraCharacterBase::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect> GameplayEffectClass,const float Level) const 
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass,Level,ContextHandle);
	const FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void AAuraCharacterBase::InitializeDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttributes,1.f);
	ApplyEffectToSelf(DefaultSecondaryAttributes,1.f);
	ApplyEffectToSelf(DefaultVitalAttributes,1.f);
}

void AAuraCharacterBase::AddCharacterAbilities()
{
	UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority()) return;
	
	AuraASC->AddCharacterAbilities(StartupAbilities);
}

void AAuraCharacterBase::Dissolve()
{
	if (IsValid(DissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMatInst = UMaterialInstanceDynamic::Create(DissolveMaterialInstance,this);
		GetMesh()->SetMaterial(0,DynamicMatInst);
		StartDissolveTimeline(DynamicMatInst);
	}
	if (IsValid(WeaponDissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMatInst = UMaterialInstanceDynamic::Create(WeaponDissolveMaterialInstance,this);
		Weapon->SetMaterial(0,DynamicMatInst);
		StartWeaponDissolveTimeline(DynamicMatInst);
	}
}


