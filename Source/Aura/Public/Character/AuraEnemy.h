// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "AuraEnemy.generated.h"


class AAuraAIController;
class UBehaviorTree;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class AURA_API AAuraEnemy : public AAuraCharacterBase,public IEnemyInterface
{
	GENERATED_BODY()
	
protected:
	
	
	virtual void InitAbilityActorInfo() override;
	
	virtual void InitializeDefaultAttributes() const override;
	
public:
	virtual void BeginPlay() override;	
	
	AAuraEnemy();
	
	virtual void PossessedBy(AController* NewController) override;
	
	/* Enemy Interface */
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
	
	virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override;
	virtual AActor* GetCombatTarget_Implementation() override ;
	/* End Enemy Interface */
	
	/* Combat Interface*/
	virtual int32 GetAuraCharacterLevel_Implementation() const override;
	
	virtual void Die() override;
	/*End Combat Interface*/
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
	virtual bool IsPlayer_Implementation() const override;
	
	
	UPROPERTY(BlueprintReadOnly ,Category = "Combat")
	bool bHitReacting = false;
	
	UPROPERTY(BlueprintReadOnly ,Category = "Combat")
	bool bAttacking = false;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly ,Category = "Combat")
	float BaseWalkSpeed = 250.f;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly ,Category = "Combat")
	float LifeSpan = 5.f;
	
	UPROPERTY(BlueprintReadOnly ,Category = "Combat")
	TObjectPtr<AActor> CombatTarget;
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly , Category = "Character Class Defaults")
	int32 Level = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly , Category = "Character Class Defaults")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> HealthBar;
	
	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
	
	UPROPERTY()
	 TObjectPtr<AAuraAIController> AuraAIController;
	
};
