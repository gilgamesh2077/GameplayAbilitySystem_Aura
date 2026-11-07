// See you in the battle


#include "AbilitySystem/Ability/AuraSummonAbility.h"
#include "Interaction/CombatInterface.h"


void UAuraSummonAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 提交冷却（若配置了 CooldownGameplayEffectClass，此处会施加冷却 GE；
	// 若正处于冷却中，CommitAbility 返回 false，直接结束技能）
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 交给蓝图 Event ActivateAbility 执行召唤逻辑
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

TArray<FVector> UAuraSummonAbility::GetSpawnLocations()
{
	
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();
	int32 SpawnNum = NumMinions;
	if (GetAvatarActorFromActorInfo()->Implements<UCombatInterface>())
	SpawnNum = FMath::Max(1,NumMinions - ICombatInterface::Execute_GetMinionsCount(GetAvatarActorFromActorInfo()));
	const float DeltaSpread = SpawnSpread /SpawnNum;
	
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread / 2.f,FVector::UpVector);
	
	TArray<FVector> SpawnLocations;
	for (int32 i = 0; i< SpawnNum; i++)
	{
		const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i,FVector::UpVector);
		FVector ChosenSpawnLocation = Location + Direction * FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
		
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit,ChosenSpawnLocation + FVector(0.f,0.f,400.f),ChosenSpawnLocation - FVector(0.f,0.f,400.f),ECC_Visibility );
		if (Hit.bBlockingHit)
		{
			ChosenSpawnLocation = Hit.ImpactPoint;
		}
		SpawnLocations.Add(ChosenSpawnLocation);
	}
	if (GetAvatarActorFromActorInfo()->Implements<UCombatInterface>()) ICombatInterface::Execute_SetMinionsCount(GetAvatarActorFromActorInfo(),
		SpawnNum + ICombatInterface::Execute_GetMinionsCount(GetAvatarActorFromActorInfo()));
	
	return SpawnLocations;
}

TSubclassOf<APawn> UAuraSummonAbility::GetRandomMinionClass()
{
	const int32 Selection = FMath:: RandRange(0,MinionsClasses.Num()-1);
	return MinionsClasses[Selection];
}
