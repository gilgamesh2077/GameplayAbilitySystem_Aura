// See you in the battle

#include "Animation/AuraAnimNotifyState_ComboAttackWindow.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UAuraAnimNotifyState_ComboAttackWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;
	if (UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner())))
	{
		ASC->BeginComboAttackWindow();
	}
}

void UAuraAnimNotifyState_ComboAttackWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;
	if (UAuraAbilitySystemComponent* ASC = Cast<UAuraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner())))
	{
		ASC->EndComboAttackWindow();
		FGameplayEventData Payload;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			MeshComp->GetOwner(), FAuraGameplayTags::Get().Event_Montage_ComboWindowClosed, Payload);
	}
}
