#include "Character/Shinbi/GAS/GA_Shinbi_BasicAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "GameplayTagsManager.h"
#include "Interface/Interface_CharacterAction.h"

void UGA_Shinbi_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	AvatarActor = GetAvatarActorFromActorInfo();

	HitActors.Empty();

	bComboInputEnabled = false;

	if (IInterface_CharacterAction* CharInterface = Cast<IInterface_CharacterAction>(AvatarActor))
	{
		CharInterface->RequestSnapToCameraDirection();
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayComboMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboMontage);
		PlayComboMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayComboMontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayComboMontageTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayComboMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayComboMontageTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitComboChangeEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetComboChangeEventTag(), nullptr, false, false);
		WaitComboChangeEventTask->EventReceived.AddDynamic(this, &ThisClass::ComboChangedEventReceived);
		WaitComboChangeEventTask->ReadyForActivation();
	}

	if (K2_HasAuthority())
	{
		UAbilityTask_WaitGameplayEvent* WaitTargetingEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetComboTargetEventTag());
		WaitTargetingEventTask->EventReceived.AddDynamic(this, &ThisClass::DoDamage);
		WaitTargetingEventTask->ReadyForActivation();
	}

	SetupWaitComboInputPress();
}

FGameplayTag UGA_Shinbi_BasicAttack::GetComboChangeEventTag()
{
	return FGameplayTag::RequestGameplayTag("ability.combo");
}

FGameplayTag UGA_Shinbi_BasicAttack::GetComboChangeEventEndTag()
{
	return FGameplayTag::RequestGameplayTag("ability.combo.end");
}

FGameplayTag UGA_Shinbi_BasicAttack::GetComboTargetEventTag()
{
	return FGameplayTag::RequestGameplayTag("ability.target.damage");
}

void UGA_Shinbi_BasicAttack::SetupWaitComboInputPress()
{
	UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputPress->OnPress.AddDynamic(this, &ThisClass::HandleInputPress);
	WaitInputPress->ReadyForActivation();
}

void UGA_Shinbi_BasicAttack::HandleInputPress(float TimeWaited)
{
	SetupWaitComboInputPress();
	TryCommitCombo();
}

void UGA_Shinbi_BasicAttack::TryCommitCombo()
{
	if (NextComboName == NAME_None)	return;
	if (!bComboInputEnabled) return;

	UAnimInstance* OwnerAnimInstance = GetOwnerAnimInstance();
	if (!OwnerAnimInstance)	return;

	// 이 방식은 다음 섹션 몽타주를 예약해두는 방식이라 몽타주가 끝날때까지 기다렸다가 다음 몽타주를 실행해서 딜레이가 있는것처럼 보임
	//OwnerAnimInstance->Montage_SetNextSection(OwnerAnimInstance->Montage_GetCurrentSection(ComboMontage), NextComboName, ComboMontage);

	HitActors.Empty();
	OwnerAnimInstance->Montage_JumpToSection(NextComboName, ComboMontage);
	bComboInputEnabled = false;
	
	if (IInterface_CharacterAction* CharInterface = Cast<IInterface_CharacterAction>(AvatarActor))
	{
		CharInterface->RequestSnapToCameraDirection();
	}
}

void UGA_Shinbi_BasicAttack::ComboChangedEventReceived(FGameplayEventData Data)
{
	FGameplayTag EventTag = Data.EventTag;

	// 테스트 해보니 사실상 의미 없는 코드, 어차피 몽타주 하나가 끝나면 자동으로 초기화 됨
	if (EventTag == GetComboChangeEventEndTag())
	{
		NextComboName = NAME_None;
		return;
	}

	TArray<FName> TagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, TagNames);
	NextComboName = TagNames.Last();
	bComboInputEnabled = true;
}

void UGA_Shinbi_BasicAttack::DoDamage(FGameplayEventData Data)
{
	AActor* HitActor = const_cast<AActor*>(Data.Target.Get());
	if (!HitActor) return;

	if (HitActors.Contains(HitActor)) return;
	HitActors.Add(HitActor);

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FGameplayEffectContextHandle EffectContext = MakeEffectContext(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());
	
	if (Data.TargetData.IsValid(0))
	{
		const FHitResult* HitResult = Data.TargetData.Get(0)->GetHitResult();
		if (HitResult)
		{
			EffectContext.AddHitResult(*HitResult);
		}
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(BasicAttack_DamageEffect, GetAbilityLevel(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo()));
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetContext(EffectContext);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		-10.f
	);

	FGameplayAbilityTargetDataHandle TargetDataHandle = Data.TargetData;
	
	(void)ApplyGameplayEffectSpecToTarget(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, SpecHandle, TargetDataHandle);
}
