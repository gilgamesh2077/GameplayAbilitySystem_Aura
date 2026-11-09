// See you in the battle


#include "AbilitySystem/AuraAbilitySystemComponent.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/Ability/AuraGameplayAbility.h"
#include "Aura/AuraLogChannels.h"


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

		const bool bInvoked = InvokeReplicatedEvent(
			EAbilityGenericReplicatedEvent::InputPressed,
			AbilitySpec.Handle,
			AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
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

