#include "Character/Shinbi/GAS/GA_Shinbi_QSkill.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Character/Shinbi/Wolf/Wolf.h"
#include "Components/DecalComponent.h"
#include "GAS/WAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"


void UGA_Shinbi_QSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CanActivateAbility(Handle, ActorInfo))
	{
		K2_EndAbility();
		return;
	}

	if (ActorInfo->IsLocallyControlled())
	{
		SpawnDashRangeDecal();
	}

	FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (Spec && !Spec->InputPressed)
	{
		OnInputReleased(0.f);	// 키를 즉시 떼는 경우
	}
	else
	{
		KeydownTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, QSkill_Keydown_Montage);
		KeydownTask->ReadyForActivation();
		
		UAbilityTask_WaitInputRelease* WaitReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
		WaitReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
		WaitReleaseTask->ReadyForActivation();
	}
}

void UGA_Shinbi_QSkill::SpawnDashWolf(FGameplayEventData Data)
{
	if (K2_HasAuthority())
	{
		FVector OwnerCharacterLocation = Avatar->GetActorLocation();
		FRotator OwnerCharacterRotation = Avatar->GetActorRotation();
		FVector ForwardVector = Avatar->GetActorForwardVector();

		float SpawDistance = 100.f;
		FVector SpawnLocation = OwnerCharacterLocation + (ForwardVector * SpawDistance);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Cast<APawn>(Avatar);
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AWolf* Wolf = GetWorld()->SpawnActor<AWolf>(
			WolfClass,
			SpawnLocation,
			OwnerCharacterRotation,
			SpawnParams
		);

		if (Wolf)
		{
			Wolf->LaunchWolf(Avatar);
		}
	}

	if (ASC) ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
}

void UGA_Shinbi_QSkill::OnInputReleased(float TimeHeld)
{
	if (IInterface_CharacterAction* CharInterface = Cast<IInterface_CharacterAction>(GetAvatarActorFromActorInfo()))
	{
		CharInterface->RequestSnapToCameraDirection(20.f);
	}
	
	if (ASC) ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
	
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	
	if (Avatar->SkillForwardDecal)
	{
		Avatar->SkillForwardDecal->DestroyComponent();
		Avatar->SkillForwardDecal = nullptr;
	}
	
	if (KeydownTask)
	{
		KeydownTask->EndTask();
		KeydownTask = nullptr;
	}
	
	if (HasAuthorityOrPredictionKey(GetCurrentActorInfo(), &ActivationInfo))
	{
		if (Avatar)
		{
			AWCharacterBase* PlayerAvatar = Cast<AWCharacterBase>(Avatar);
			if (PlayerAvatar)
			{
				FRotator CameraRot = PlayerAvatar->GetController()->GetControlRotation();
				PlayerAvatar->SetActorRotation(FRotator(0, CameraRot.Yaw, 0));
			}
		}
		
		UAbilityTask_PlayMontageAndWait* PlayQSkillMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, QSkill_SpawnDashWolf_Montage);
		PlayQSkillMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->ReadyForActivation();
	}

	if (HasAuthorityOrPredictionKey(GetCurrentActorInfo(), &ActivationInfo))
	{
		UAbilityTask_WaitGameplayEvent* WaitSpawnWolfEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetQSkillSpawnWolfEventTag());
		WaitSpawnWolfEventTask->EventReceived.AddDynamic(this, &ThisClass::SpawnDashWolf);
		WaitSpawnWolfEventTask->ReadyForActivation();
	}

	ApplyCooldown();
}

FGameplayTag UGA_Shinbi_QSkill::GetQSkillSpawnWolfEventTag()
{
	return FGameplayTag::RequestGameplayTag("ability.shinbi.qskill.spawn");
}

void UGA_Shinbi_QSkill::SpawnDashRangeDecal()
{	
	if (!Avatar) return;

	FVector SpawnLocation = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 700.f;
	FRotator SpawnRotation = Avatar->GetActorRotation();

	SpawnRotation.Pitch -= 90.f;
	SpawnRotation.Yaw -= 90.f;

	SpawnedDecal = UGameplayStatics::SpawnDecalAtLocation(
		GetWorld(),
		SkillRangeDecalMaterial,
		FVector(100.f, 600.f, 50.f),
		SpawnLocation,
		SpawnRotation
	);

	if (SpawnedDecal)
	{
		Avatar->SkillForwardDecal = SpawnedDecal;
	}
}

void UGA_Shinbi_QSkill::ApplyCooldown()
{
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffectClass, 1.f);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("ability.data.cooldown"), CooldownTime);

		ApplyGameplayEffectSpecToOwner(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfoRef(),
			SpecHandle
		);
	}
}

void UGA_Shinbi_QSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.charging"));
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Shinbi_QSkill::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	if (Avatar->SkillForwardDecal)
	{
		Avatar->SkillForwardDecal->DestroyComponent();
		Avatar->SkillForwardDecal = nullptr;
	}

	K2_EndAbility();
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}
