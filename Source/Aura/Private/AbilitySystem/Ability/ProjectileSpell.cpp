// See you in the battle


#include "AbilitySystem/Ability/ProjectileSpell.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Interaction/CombatInterface.h"
#include "Actor/AuraProjectile.h"

void UProjectileSpell::SpawnProjectile_Implementation(FVector AimLocation)
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return;

	const UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor);
	if (!SourceASC) return;

	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(
		AvatarActor,
		FAuraGameplayTags::Get().Montage_Attack_Weapon);

	FTransform SpawnTransform;
	AimLocation.Z = SocketLocation.Z;
	FRotator Rotation = (AimLocation - SocketLocation).Rotation();
	Rotation.Pitch = 0.0f;
	SpawnTransform.SetRotation(Rotation.Quaternion());
	SpawnTransform.SetLocation(SocketLocation);

	AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
		ProjectileClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		Cast<APawn>(AvatarActor),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
	EffectContextHandle.SetAbility(this);
	EffectContextHandle.AddSourceObject(Projectile);
	TArray<TWeakObjectPtr<AActor>> Actors;
	Actors.Add(Projectile);
	EffectContextHandle.AddActors(Actors);

	const FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContextHandle);

	for (const auto& Pair : DamageTypes)
	{
		const float ScaleDamage = Pair.Value.GetValueAtLevel(GetAbilityLevel());
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Pair.Key, ScaleDamage);
	}

	Projectile->DamageEffectSpecHandle = SpecHandle;
	Projectile->FinishSpawning(SpawnTransform);
}


