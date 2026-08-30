#include "Character/Wraith/GAS/GA_Wraith_RMSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Character/WCharacterBase.h"
#include "GAS/WAbilitySystemComponent.h"


void UGA_Wraith_RMSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CanActivateAbility(Handle, ActorInfo))
	{
		K2_EndAbility();
		return;
	}

	if (K2_HasAuthority())
	{
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
		if (!SourceASC) return;

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Wraith_RMSkill_Effect, 1.f, EffectContext);
		if (!SpecHandle.IsValid()) return;

		EffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* StartSnipeMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, StartSnipeModeMontage);
		StartSnipeMontageTask->ReadyForActivation();

		FGameplayCueParameters CueParams;
		ASC->AddGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.wraith.rmskill.loadscope")), CueParams);
		
		UAbilityTask_WaitInputPress* WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
		WaitInputTask->OnPress.AddDynamic(this, &ThisClass::OnCancelRMSkillInput);
		WaitInputTask->ReadyForActivation();
	}
}

void UGA_Wraith_RMSkill::OnCancelRMSkillInput(float TimeHeld)
{
	if (!EffectHandle.IsValid()) return;

	BP_RemoveGameplayEffectFromOwnerWithHandle(EffectHandle);
	ASC->RemoveGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.wraith.rmskill.loadscope")));

	K2_EndAbility();
}

void UGA_Wraith_RMSkill::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	OnCancelRMSkillInput(0.f);

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}
