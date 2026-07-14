// See you in the battle

#include "AbilitySystem/AuraAssetManager.h"
#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "Engine/Engine.h"

UAuraAssetManager& UAuraAssetManager::Get()
{
	check(GEngine);
	return *CastChecked<UAuraAssetManager>(GEngine->AssetManager);
}

void UAuraAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FAuraGameplayTags::InitializeNativeGameplayTags();
	UAbilitySystemGlobals::Get().InitGlobalData();
}
