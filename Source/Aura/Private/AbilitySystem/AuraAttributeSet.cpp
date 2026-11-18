// See you in the battle


#include "AbilitySystem/AuraAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Aura/AuraLogChannels.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"

UAuraAttributeSet::UAuraAttributeSet()
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	
	// Primary Attributes
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Strength,GetStrengthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Intelligence,GetIntelligenceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Resilience,GetResilienceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Vigor,GetVigorAttribute);

	// Secondary Attributes
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Armor,GetArmorAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ArmorPenetration,GetArmorPenetrationAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BlockChance,GetBlockChanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitChance,GetCriticalHitChanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitDamage,GetCriticalHitDamageAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitResistance,GetCriticalHitResistanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthRegeneration,GetHealthRegenerationAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaRegeneration,GetManaRegenerationAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealth,GetMaxHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxMana,GetMaxManaAttribute);
	
	// Resistance Attributes
	
	TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Arcane,GetArcaneResistanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Fire,GetFireResistanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Lightning,GetLightningResistanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Physical,GetPhysicalResistanceAttribute);
}

void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Primary Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Strength , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Intelligence , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Resilience , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Vigor , COND_None,REPNOTIFY_Always);
	//Vital Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Health , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Mana , COND_None,REPNOTIFY_Always);
	
	
	//Secondary Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,Armor , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,ArmorPenetration , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,BlockChance , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,CriticalHitChance , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,CriticalHitDamage , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,CriticalHitResistance , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,HealthRegeneration , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,ManaRegeneration , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,MaxHealth , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,MaxMana , COND_None,REPNOTIFY_Always);
	
	// Resistance Attributes
	
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,FireResistance , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,ArcaneResistance , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,LightningResistance , COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet ,PhysicalResistance , COND_None,REPNOTIFY_Always);
}

void UAuraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
}

void UAuraAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const
{
	//Source = causer of the effect ,Target = target of the effect (owner of this AS)
	
	const FGameplayEffectContextHandle EffectContextHandle = Data.EffectSpec.GetContext();
	Props.EffectContext = EffectContextHandle;
	UAbilitySystemComponent* SourceASC = EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();
	Props.SourceAbilitySystemComponent = SourceASC;
	
	if (IsValid(SourceASC) && SourceASC->AbilityActorInfo.IsValid() && SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		AActor* SourceAvatarActor = SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceAvatarActor = SourceAvatarActor;
		AController* SourceController = SourceASC->AbilityActorInfo->PlayerController.Get();
		if (SourceController == nullptr && SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(SourceAvatarActor))
			{
				SourceController = Pawn->GetController(); 
			}
		}
		Props.SourceController = SourceController;
		if (SourceController)
		{
			ACharacter* SourceCharacter = Cast<ACharacter>(SourceController->GetPawn());
			Props.SourceCharacter = SourceCharacter;
		}
	}
	
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		AActor* TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		
		Props.TargetAvatarActor = TargetAvatarActor;
		
		AController* TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		if (TargetController == nullptr && TargetAvatarActor != nullptr)
		{
			if (const APawn* TargetPawn = Cast<APawn>(TargetAvatarActor))
			{
				TargetController = TargetPawn->GetController(); // 敌人通常是 null，合法
			}
		}
		Props.TargetController = TargetController;
	
		ACharacter* TargetCharacter = Cast<ACharacter>(TargetAvatarActor);
		if (TargetCharacter == nullptr && TargetController != nullptr)
		{
			TargetCharacter = Cast<ACharacter>(TargetController->GetPawn());
		}
		Props.TargetCharacter = TargetCharacter;
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetAvatarActor);
		
		Props.TargetAbilitySystemComponent = TargetASC;
	}
	
	
}

void UAuraAttributeSet::SendXPEvent(const FEffectProperties& Props)
{
	ACharacter* Character = Props.TargetCharacter;
	if (Character->Implements<UCombatInterface>())
	{
		const int32 CharacterLevel = ICombatInterface::Execute_GetAuraCharacterLevel(Character);
		ECharacterClass CharacterClass = ICombatInterface::Execute_GetCharacterClass(Character);
		const int32 XPReward = UAuraAbilitySystemLibrary::GetXPRewardForClassAndLevel(Props.TargetCharacter,CharacterClass,CharacterLevel);
		
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags.Attributes_Meta_IncomingXP;
		Payload.EventMagnitude = XPReward;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Props.SourceAvatarActor,GameplayTags.Attributes_Meta_IncomingXP,Payload);
	}
}

void UAuraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	
	FEffectProperties Props;
	SetEffectProperties(Data,Props);
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(),0.f,GetMaxHealth()));
	}
	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(),0.f,GetMaxMana()));
	}
	
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (LocalIncomingDamage > 0.f)
		{
			const float NewHealth = GetHealth() - LocalIncomingDamage;
			SetHealth(FMath::Clamp(NewHealth,0.f,GetMaxHealth()));
			
			const bool bFatal = NewHealth <= 0.f;
			
			if (bFatal)
			{
				ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor);
				if (CombatInterface)
				{
					CombatInterface->Die();
					SendXPEvent(Props);
				}
			}
			else
			{
				FGameplayTagContainer TagContainer;
				TagContainer.AddTag(FAuraGameplayTags::Get().Effects_HitReact);
				const bool bActivated = Props.TargetAbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
				UE_LOG(LogAura, Warning, TEXT("[HitReact] TryActivate = %s"), bActivated ? TEXT("true") : TEXT("false"));
				
			}
			
			//show damage
			if (Props.SourceCharacter != Props.TargetCharacter)
			{
				if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.SourceController))
				{
					const bool bBlock = UAuraAbilitySystemLibrary::IsBlockedHit(Props.EffectContext);
					const bool bCriticalHit = UAuraAbilitySystemLibrary::IsCriticalHit(Props.EffectContext);
					PC->ShowDamageNumber(LocalIncomingDamage,Props.TargetCharacter ,bBlock,bCriticalHit);
					return;
				}
				if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.TargetController))
				{
					const bool bBlock = UAuraAbilitySystemLibrary::IsBlockedHit(Props.EffectContext);
					const bool bCriticalHit = UAuraAbilitySystemLibrary::IsCriticalHit(Props.EffectContext);
					PC->ShowDamageNumber(LocalIncomingDamage,Props.TargetCharacter ,bBlock,bCriticalHit);
				}
			}
		}
	}
	
	if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		const float LocalIncomingXP = GetIncomingXP();
		SetIncomingXP(0.f);
		
		if (Props.SourceCharacter->Implements<UPlayerInterface>() && Props.SourceCharacter->Implements<UCombatInterface>())
		{
			const int32 CurrentLevel = ICombatInterface::Execute_GetAuraCharacterLevel(Props.SourceCharacter);
			const int32 CurrentXP = IPlayerInterface::Execute_GetXP(Props.SourceCharacter);
			IPlayerInterface::Execute_AddToXP(Props.SourceCharacter,LocalIncomingXP);
			const int32 NewLevel = IPlayerInterface::Execute_FindLevelForXP(Props.SourceCharacter,CurrentXP + LocalIncomingXP);
			const int32 NumLevelUps = NewLevel - CurrentLevel;
			int32 AttributePointsReward = 0;
			int32 SpellPointsReward = 0;
			for (int32 i = 0; i<NumLevelUps ;++i)
			{
				 AttributePointsReward+= IPlayerInterface::Execute_GetAttributePointsReward(Props.SourceCharacter,CurrentLevel+i);
				 SpellPointsReward+= IPlayerInterface::Execute_GetSpellPointsReward(Props.SourceCharacter,CurrentLevel+i);
			}
			
			if (NumLevelUps > 0)
			{
				IPlayerInterface::Execute_AddToPlayerLevel(Props.SourceCharacter,NumLevelUps);
				IPlayerInterface::Execute_AddToAttributePoints(Props.SourceCharacter,AttributePointsReward);
				IPlayerInterface::Execute_AddToSpellPoints(Props.SourceCharacter,SpellPointsReward);
				
				bTopOffHealth = true;
				bTopOffMana = true;
				IPlayerInterface::Execute_LevelUp(Props.SourceCharacter);
			}
			
		}
		
	}
}

void UAuraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetMaxHealthAttribute() && bTopOffHealth)
	{
		SetHealth(GetMaxHealth());
		bTopOffHealth = false;
	}
	
	if (Attribute == GetMaxManaAttribute() && bTopOffMana)
	{
		SetMana(GetMaxMana());
		bTopOffMana = false;
	}
}

void UAuraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Health , OldHealth);
}

void UAuraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxHealth , OldMaxHealth);
}

void UAuraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Mana , OldMana);
}

void UAuraAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , MaxMana , OldMaxMana);
}

void UAuraAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , Strength , OldStrength);
}

void UAuraAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , Intelligence , OldIntelligence);
}

void UAuraAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldResilience) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , Resilience , OldResilience);
}

void UAuraAttributeSet::OnRep_Vigor(const FGameplayAttributeData& OldVigor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , Vigor , OldVigor);
}

void UAuraAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , Armor , OldArmor);
}

void UAuraAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , ArmorPenetration , OldArmorPenetration);
}

void UAuraAttributeSet::OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , BlockChance , OldBlockChance);
}

void UAuraAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , CriticalHitChance , OldCriticalHitChance);
}

void UAuraAttributeSet::OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , CriticalHitDamage , OldCriticalHitDamage);
}

void UAuraAttributeSet::OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , CriticalHitResistance , OldCriticalHitResistance);
}

void UAuraAttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , HealthRegeneration , OldHealthRegeneration);
}

void UAuraAttributeSet::OnRep_ManaRegeneration(const FGameplayAttributeData& OldManaRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , ManaRegeneration , OldManaRegeneration);
}

void UAuraAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldFireResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , FireResistance , OldFireResistance);
}

void UAuraAttributeSet::OnRep_LightningResistance(const FGameplayAttributeData& OldLightningResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , LightningResistance , OldLightningResistance);
}

void UAuraAttributeSet::OnRep_ArcaneResistance(const FGameplayAttributeData& OldArcaneResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , ArcaneResistance , OldArcaneResistance);
}

void UAuraAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet , PhysicalResistance , OldPhysicalResistance);
}



