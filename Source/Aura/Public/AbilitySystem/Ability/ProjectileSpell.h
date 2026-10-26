// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "AuraDamageGameplayAbility.h"
#include "ProjectileSpell.generated.h"

class AAuraProjectile;

/**
 * 投射物法术基类：只负责"朝一个瞄准点发射投射物"，并把伤害 Spec 交给投射物。
 * 瞄准点从哪来由子类决定（玩家 = 鼠标，敌人 = CombatTarget）。
 */
UCLASS()
class AURA_API UProjectileSpell : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<AAuraProjectile> ProjectileClass;
	
	UFUNCTION(BlueprintCallable , BlueprintNativeEvent)
	void SpawnProjectile(FVector AimLocation);
};
