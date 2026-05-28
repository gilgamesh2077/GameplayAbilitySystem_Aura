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
	
	
	/* 
	 *Primary Attribute 
	 */
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Strength , Category = "Primary Attributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Strength)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Intelligence , Category = "Primary Attributes")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Intelligence)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Resilience , Category = "Primary Attributes")
	FGameplayAttributeData Resilience;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Resilience)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Vigor , Category = "Primary Attributes")
	FGameplayAttributeData Vigor;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Vigor)
	
	/*
	 * Secondary Attributes
	 */
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Armor , Category = "Secondary Attributes")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Armor)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_ArmorPenetration , Category = "Secondary Attributes")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, ArmorPenetration)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_BlockChance , Category = "Secondary Attributes")
	FGameplayAttributeData BlockChance;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, BlockChance)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_CriticalHitChance , Category = "Secondary Attributes")
	FGameplayAttributeData CriticalHitChance;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, CriticalHitChance)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_CriticalHitDamage , Category = "Secondary Attributes")
	FGameplayAttributeData CriticalHitDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, CriticalHitDamage)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_CriticalHitResistance , Category = "Secondary Attributes")
	FGameplayAttributeData CriticalHitResistance;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, CriticalHitResistance)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_HealthRegeneration , Category = "Secondary Attributes")
	FGameplayAttributeData HealthRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, HealthRegeneration)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_ManaRegeneration , Category = "Secondary Attributes")
	FGameplayAttributeData ManaRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, ManaRegeneration)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_MaxHealth , Category = "Secondary Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, MaxHealth)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_MaxMana , Category = "Secondary Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, MaxMana)
	
	/*
	 * Vital Attribute
	 */
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Health , Category = "Vital Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Health)
	
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Mana , Category = "Vital Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS_BASIC(UAuraAttributeSet, Mana)
	
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;
	
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldMana) const;
	
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const;
	
	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldStrength) const;
	
	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const;
	
	UFUNCTION()
	void OnRep_Resilience(const FGameplayAttributeData& OldResilience) const;
	
	UFUNCTION()
	void OnRep_Vigor(const FGameplayAttributeData& OldVigor) const;
	
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldArmor) const;
	
	UFUNCTION()
	void OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const;
	
	UFUNCTION()
	void OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance) const;
	
	UFUNCTION()
	void OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const;
	
	UFUNCTION()
	void OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const;
	
	UFUNCTION()
	void OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const;
	
	UFUNCTION()
	void OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const;
	
	UFUNCTION()
	void OnRep_ManaRegeneration(const FGameplayAttributeData& OldManaRegeneration) const;
	
private:
	 void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const ;
};
