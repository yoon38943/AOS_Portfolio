#include "GAS/WGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Character/WCharacterBase.h"
#include "Character/WCharAnimInstance.h"
#include "PersistentGame/GamePlayerController.h"

class UAnimInstance* UWGameplayAbility::GetOwnerAnimInstance() const
{
	USkeletalMeshComponent* OwnerSkeletalMeshComp = GetOwningComponentFromActorInfo();
	if (OwnerSkeletalMeshComp)
	{
		return OwnerSkeletalMeshComp->GetAnimInstance();
	}
	return nullptr;
}

void UWGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		AWCharacterBase* Character = Cast<AWCharacterBase>(GetAvatarActorFromActorInfo());
		if (!Character) return;
	}

	if (K2_HasAuthority())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		if (!ContextHandle.IsValid()) return;
		
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CombatEffectClass, 1.f, ContextHandle);
		if (!SpecHandle.IsValid()) return;

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
