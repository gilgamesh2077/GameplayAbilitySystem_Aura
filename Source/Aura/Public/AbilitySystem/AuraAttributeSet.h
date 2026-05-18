// See you in the battle

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AuraAttributeSet.generated.h"


#define ATTRIBUTE_ACCESSORS_BASIC(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


USTRUCT()
struct FEffectProperties
{
	GENERATED_BODY()
	
	FEffectProperties() = default;
	FEffectProperties(UAbilitySystemComponent* AbilitySystemComponent,
	AActor* AvatarActor,
	AController* Controller,
	ACharacter* Character,
	const FGameplayEffectContextHandle& EffectContext): SourceAbilitySystemComponent(AbilitySystemComponent),
	SourceAvatarActor(AvatarActor) ,
	SourceController(Controller), 
	SourceCharacter(Character),
	TargetAbilitySystemComponent(AbilitySystemComponent),
	TargetAvatarActor(AvatarActor) ,
	TargetController(Controller), 
	TargetCharacter(Character),
	EffectContext(EffectContext) {}
	
	UPROPERTY()
	UAbilitySystemComponent* SourceAbilitySystemComponent = nullptr;
	UPROPERTY()
	AActor* SourceAvatarActor = nullptr;
	UPROPERTY()
	AController* SourceController = nullptr;
	UPROPERTY()
	ACharacter* SourceCharacter = nullptr;
	
	UPROPERTY()
	UAbilitySystemComponent*  TargetAbilitySystemComponent = nullptr;
	UPROPERTY()
	AActor*  TargetAvatarActor = nullptr;
	UPROPERTY()
	AController*  TargetController = nullptr;
	UPROPERTY()
	ACharacter*  TargetCharacter = nullptr;
	
	FGameplayEffectContextHandle EffectContext;
};


/**
 * 
 */
UCLASS()
class AURA_API UAuraAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UAuraAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Health , Category = "Vital Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Health)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_MaxHealth , Category = "Vital Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, MaxHealth)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Mana , Category = "Vital Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Mana)
    	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_MaxMana , Category = "Vital Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, MaxMana)
	
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;
	
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldMana) const;
	
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const;
	
private:
	 void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const ;
};
