// See you in the battle


#include "Player/AuraPlayerState.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Net/UnrealNetwork.h"


void AAuraPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass,Level);
	DOREPLIFETIME(ThisClass,XP);
	DOREPLIFETIME(ThisClass,AttributePoints);
	DOREPLIFETIME(ThisClass,SpellPoints);
}

AAuraPlayerState::AAuraPlayerState()
{
	SetNetUpdateFrequency(100.f);
	
	AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AAuraPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAuraPlayerState::On_RepLevel(int32 OldLevel) const
{
	OnLevelChangedDelegate.Broadcast(Level);
}

void AAuraPlayerState::On_RepXP(int32 OldXP) const
{
	OnXPChangedDelegate.Broadcast(XP);
}

void AAuraPlayerState::On_RepAttributePoints(int32 OldAttributePoints) const
{
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AAuraPlayerState::On_RepSpellPoints(int32 OldAttributePoints) const
{
	 OnSpellPointsChangedDelegate.Broadcast(SpellPoints);
}
