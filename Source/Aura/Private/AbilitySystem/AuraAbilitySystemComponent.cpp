// See you in the battle


#include "AbilitySystem/AuraAbilitySystemComponent.h"



UAuraAbilitySystemComponent::UAuraAbilitySystemComponent()
{
	
	PrimaryComponentTick.bCanEverTick = true;


}

void UAuraAbilitySystemComponent::AbilitySystemInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this,&UAuraAbilitySystemComponent::EffectApplied);
}


void UAuraAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();


	
}

void UAuraAbilitySystemComponent::EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayEffectSpec& EffectSpec,
		FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	EffectAssetTags.Broadcast(TagContainer);
	
}


void UAuraAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


}

