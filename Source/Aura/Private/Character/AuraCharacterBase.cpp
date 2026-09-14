// See you in the battle


#include "Character/AuraCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Player/AuraPlayerController.h"
#include "MotionWarpingComponent.h"
#include "Aura/Aura.h"

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

FVector AAuraCharacterBase::GetCombatSocketLocation() const
{
	return Weapon->GetSocketLocation(WeaponTipSocketName);
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

void AAuraCharacterBase::UpdateFacingRotationFromLocation(const FVector& Location)
{
	UpdateAttackWarpTarget(Location);
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



