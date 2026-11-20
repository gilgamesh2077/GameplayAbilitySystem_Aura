// See you in the battle


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Aura/AuraLogChannels.h"
#include "Player/AuraPlayerState.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
	OnMaxHealthChanged.Broadcast(GetAuraAS()->GetMaxHealth());
	OnHealthChanged.Broadcast(GetAuraAS()->GetHealth());
	OnMaxManaChanged.Broadcast(GetAuraAS()->GetMaxMana());
	OnManaChanged.Broadcast(GetAuraAS()->GetMana());
	OnXPPercentChangedDelegate.Broadcast(GetXPBarPercentForXP(AuraPlayerState->GetXP()));
	OnPlayerLevelChangedDelegate.Broadcast(AuraPlayerState->GetPlayerLevel());
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	GetAuraPS()->OnXPChangedDelegate.AddUObject(this,&UOverlayWidgetController::OnXPChanged);
	
	GetAuraPS()->OnLevelChangedDelegate.AddUObject(this,&UOverlayWidgetController::OnPlayerLevelChanged);
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnHealthChanged.Broadcast(Data.NewValue);
	});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnMaxHealthChanged.Broadcast(Data.NewValue);
	});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnManaChanged.Broadcast(Data.NewValue);
	});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAttributeSet->GetMaxManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnMaxManaChanged.Broadcast(Data.NewValue);
	});
	
	if (UAuraAbilitySystemComponent* AuraASC = GetAuraASC())
	{
		AuraASC->AbilityEquipped.AddUObject(this,&UOverlayWidgetController::OnAbilityEquipped);
		if (AuraASC->bStartupAbilitiesGiven)
		{
			BroadcastAbilityInfo();
		}
		else
		{
			AuraASC->AbilitiesGivenDelegate.AddUObject(this, &UOverlayWidgetController::BroadcastAbilityInfo);
		}
		
		AuraASC->EffectAssetTags.AddLambda([this](const FGameplayTagContainer& AssetTags)
		{
			for (const auto& Tag : AssetTags)
			{
				// For example , say tha Tag = Message.HealthPotion
				// "Message.HealthPotion" .MatchesTag("Message") will return True, "Message".MatchesTag("Message.HealthPotion") will return False
			
				const FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
				if (Tag.MatchesTag(MessageTag))
				{
				const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);
				MessageWidgetRowDelegate.Broadcast(*Row);
				}
			}
		});
	}
}

void UOverlayWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status,
	const FGameplayTag& Slot, const FGameplayTag& PreviousSlot)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	FAuraAbilityInfo LastSlotInfo;
	LastSlotInfo.StatusTag = GameplayTags.Abilities_Status_Unlocked;
	LastSlotInfo.InputTag = PreviousSlot;
	LastSlotInfo.AbilityTag = GameplayTags.Abilities_None;
	AbilityInfoDelegate.Broadcast(LastSlotInfo);
	
	FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoFromTag(AbilityTag);
	Info.StatusTag = Status;
	Info.InputTag = Slot;
	AbilityInfoDelegate.Broadcast(Info);
}

void UOverlayWidgetController::OnXPChanged(int32 NewXP)
{
	OnXPPercentChangedDelegate.Broadcast(GetXPBarPercentForXP(NewXP));
}

void UOverlayWidgetController::OnPlayerLevelChanged(int32 NewPlayerLevel) const
{
	OnPlayerLevelChangedDelegate.Broadcast(NewPlayerLevel);
}

float UOverlayWidgetController::GetXPBarPercentForXP(int32 XP)
{
	if (!GetAuraPS()) return 0.f;
	if (!IsValid(GetAuraPS()->LevelUpInfo)) return 0.f;
	const int32 Level = GetAuraPS()->LevelUpInfo->FindLevelForXP(GetAuraPS()->LevelUpInfo->LevelUpInformation,XP);
	const int32 MaxLevel = GetAuraPS()->LevelUpInfo->LevelUpInformation.Num() - 1;
	
	if (Level<= MaxLevel && Level > 0)
	{
		const int32 LevelUpRequirement = GetAuraPS()->LevelUpInfo->LevelUpInformation[Level].LevelUpRequirement;
		const int32 PreviousLevelUpRequirement = GetAuraPS()->LevelUpInfo->LevelUpInformation[Level - 1].LevelUpRequirement;
		
		const int32 DeltaLevelRequirement = LevelUpRequirement - PreviousLevelUpRequirement;
		const int32 XPForThisLevel = XP - PreviousLevelUpRequirement;
		
		const float XPBarPercent = static_cast<float>(XPForThisLevel) / static_cast<float>(DeltaLevelRequirement);
		return XPBarPercent;
	}
	UE_LOG(LogAura,Error,TEXT("Level is no Validation!"));
	return 0.f;
}
