// See you in the battle


#include "AbilitySystem/AuraAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Ability/AuraGameplayAbility.h"
#include "Aura/AuraLogChannels.h"
#include "Interaction/PlayerInterface.h"


UAuraAbilitySystemComponent::UAuraAbilitySystemComponent()
{
	
	PrimaryComponentTick.bCanEverTick = true;
	
}

void UAuraAbilitySystemComponent::AbilitySystemInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this,&UAuraAbilitySystemComponent::ClientEffectApplied);
	
}


void UAuraAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass,1);
		if (const UAuraGameplayAbility* AuraAbility =  Cast<UAuraGameplayAbility>(AbilitySpec.Ability))
		{
			//Bind InputTag
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraAbility->StartupInputTag);
			GiveAbility(AbilitySpec);
		}
	}
	bStartupAbilitiesGiven = true;
	AbilitiesGivenDelegate.Broadcast(this);
}

void UAuraAbilitySystemComponent::AddCharacterPassiveAbilities(
	const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
	for (const TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass,1);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	
	for (auto& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			if (!AbilitySpec.IsActive())
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}
		
		AbilitySpecInputPressed(AbilitySpec);

		if (!AbilitySpec.IsActive())
		{
			TryActivateAbility(AbilitySpec.Handle);
			continue;
		}

		UGameplayAbility* AbilityInstance =
			AbilitySpec.GetPrimaryInstance();
		

		if (!AbilityInstance)
		{
			continue;
		}

		// 连击窗口内：按键交给 WaitInputPress，用于排队下一段。
		// 连击已经判定结束（窗口关闭且没接上，冷却已开始）而后摇还没播完：按“重新激活”处理 ——
		// bRetriggerInstancedAbility 会先结束旧实例再执行新技能；若冷却尚未结束，
		// CanActivateAbility 会直接拒绝，旧实例与它的后摇都不受影响。
		const UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilityInstance);
		if (IsComboAttackWindowOpen() || !IsComboAttackInterruptible() || !AuraAbility || !AuraAbility->IsComboChainFinished())
		{
			InvokeReplicatedEvent(
				EAbilityGenericReplicatedEvent::InputPressed,
				AbilitySpec.Handle,
				AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
		}
		else
		{
			TryActivateAbility(AbilitySpec.Handle);
		}
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) continue;

		AbilitySpecInputReleased(AbilitySpec);
		
		if (AbilitySpec.IsActive())
		{
			if (UGameplayAbility* AbilityInstance =
				AbilitySpec.GetPrimaryInstance())
			{
				InvokeReplicatedEvent(
					EAbilityGenericReplicatedEvent::InputReleased,
					AbilitySpec.Handle,
					AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
	
}

void UAuraAbilitySystemComponent::ForEachAbility(const FForEachAbility& Delegate)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (const FGameplayAbilitySpec& AbilitySpec: GetActivatableAbilities())
	{
		if (!Delegate.ExecuteIfBound(AbilitySpec))
		{
			UE_LOG(LogAura,Error,TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
		}
	}
	
}

FGameplayTag UAuraAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability)
	{
		for (FGameplayTag Tag : AbilitySpec.Ability.Get()->AbilityTags)
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
			{
				return Tag;
			}
		}
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (FGameplayTag Tag : AbilitySpec.DynamicAbilityTags)
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("InputTag"))))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

void UAuraAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor())>0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
	}
}

void UAuraAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1.f;
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(),AttributeTag,Payload);
	
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(),-1);
	}
}

void UAuraAbilitySystemComponent::CancelComboAttackAbilities()
{
	FGameplayTagContainer ComboAttackTags;
	ComboAttackTags.AddTag(FAuraGameplayTags::Get().Combo);
	
	CancelAbilities(&ComboAttackTags);
}

void UAuraAbilitySystemComponent::BeginComboAttackWindow()
{
	++ComboAttackWindowDepth;
}

void UAuraAbilitySystemComponent::EndComboAttackWindow()
{
	ComboAttackWindowDepth = FMath::Max(0, ComboAttackWindowDepth - 1);
}

void UAuraAbilitySystemComponent::ResetComboAttackWindow()
{
	ComboAttackWindowDepth = 0;
	bComboAttackInterruptible = false;
}

bool UAuraAbilitySystemComponent::IsComboAttackWindowOpen() const
{
	return ComboAttackWindowDepth > 0;
}

void UAuraAbilitySystemComponent::SetComboAttackInterruptible(bool bInterruptible)
{
	bComboAttackInterruptible = bInterruptible;
}

bool UAuraAbilitySystemComponent::IsComboAttackInterruptible() const
{
	return bComboAttackInterruptible;
}

void UAuraAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UAuraAbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayEffectSpec& EffectSpec,
		FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	EffectAssetTags.Broadcast(TagContainer);
	
}


void UAuraAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();
	if (!bStartupAbilitiesGiven)
	{
		bStartupAbilitiesGiven = true;
		AbilitiesGivenDelegate.Broadcast(this);
	}
}

void UAuraAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

