// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuraEffectActor.generated.h"

UENUM(BlueprintType)
enum class EEffectApplicationPolicy : uint8
{
	ApplyOnOverlap,
	ApplyOnEndOverlap,
	DoNotApply
};

UENUM(BlueprintType)
enum class EEffectRemovalPolicy : uint8
{
	RemoveOnEndOverlap,
	DoNotRemove
};

class UGameplayEffect;

UCLASS()
class AURA_API AAuraEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AAuraEffectActor();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* ActorTarget, TSubclassOf<UGameplayEffect> GameplayEffectClass);
	
	UFUNCTION(BlueprintCallable)
	void OnOverlap(AActor* ActorTarget);
	
	UFUNCTION(BlueprintCallable)
	void OnEndOverlap(AActor* ActorTarget);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Applied Effects")
	bool bDestroyOnEffectRemoval=false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Applied Effects")
	EEffectApplicationPolicy InstantEffectApplicationPolicy = EEffectApplicationPolicy::DoNotApply;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly ,Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> DurationGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Applied Effects")
	EEffectApplicationPolicy DurationEffectApplicationPolicy = EEffectApplicationPolicy::DoNotApply;
	
	UPROPERTY(EditAnywhere ,BlueprintReadOnly ,Category = " Applied Effects")
	TSubclassOf<UGameplayEffect> InfiniteGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Applied Effects")
	EEffectApplicationPolicy InfiniteEffectApplicationPolicy = EEffectApplicationPolicy::DoNotApply;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Applied Effects")
	EEffectRemovalPolicy InfiniteEffectRemovalPolicy = EEffectRemovalPolicy::RemoveOnEndOverlap;
};
