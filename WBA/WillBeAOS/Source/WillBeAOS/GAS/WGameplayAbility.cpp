#include "GAS/WGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "WAbilitySystemComponent.h"
#include "Character/WCharacterBase.h"

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
		ASC = Cast<UWAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
		Avatar = Cast<AWCharacterBase>(ActorInfo->AvatarActor.Get());

		if (!ASC || !Avatar)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("ability.state.recall")));
		ASC->CancelAbilities(&CancelTags, nullptr, this);
	}

	if (K2_HasAuthority())
	{		
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		if (!ContextHandle.IsValid()) return;

		if (!CombatEffectClass) return;
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CombatEffectClass, 1.f, ContextHandle);
		if (!SpecHandle.IsValid()) return;

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
