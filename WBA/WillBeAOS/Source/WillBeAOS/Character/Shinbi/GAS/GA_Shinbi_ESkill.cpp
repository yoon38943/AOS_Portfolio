#include "Character/Shinbi/GAS/GA_Shinbi_ESkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/WCharacterBase.h"
#include "Character/Shinbi/Wolf/CirclingWolves.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Character/Shinbi/Wolf/CircleDamageField.h"
#include "GAS/WAbilitySystemComponent.h"
#include "Particles/ParticleSystemComponent.h"

void UGA_Shinbi_ESkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CanActivateAbility(Handle, ActorInfo))
	{
		K2_EndAbility();
		return;
	}
	
	if (ASC)
	{		
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) && ASC)
	{
		UAbilityTask_PlayMontageAndWait* PlayESkillMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ESKill_SpawnCircleWolves_Montage);
		PlayESkillMontageTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitSpawnCircleWolvesEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetSpawnCircleWolvesTags());
		WaitSpawnCircleWolvesEvent->EventReceived.AddDynamic(this, &ThisClass::SpawnCircleWolves);
		WaitSpawnCircleWolvesEvent->ReadyForActivation();

		// 늑대를 어빌리티에서 분리해도 파티클이나 LensFlare 때문에 남겨야함
		UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, 5.f);
		WaitTask->OnFinish.AddDynamic(this, &ThisClass::K2_EndAbility);
		WaitTask->ReadyForActivation();
		
		ASC->AddGameplayCue(GetSpawnParticleCueTags());
	}
	
	if (!K2_HasAuthority())
	{
		SpawnLensFlare();
	}
}

FGameplayTag UGA_Shinbi_ESkill::GetSpawnParticleCueTags()
{
	return FGameplayTag::RequestGameplayTag("Gameplaycue.shinbi.eskill.cast");
}

FGameplayTag UGA_Shinbi_ESkill::GetSpawnCircleWolvesTags()
{
	return FGameplayTag::RequestGameplayTag("ability.shinbi.eskill.spawn");
}

void UGA_Shinbi_ESkill::SpawnCircleWolves(FGameplayEventData Data)
{
	if (K2_HasAuthority())
	{
		if (!Avatar) return;

		int32 WolfCount = 5;
		for (int32 i = 0; i < WolfCount; i++)
		{
			float StartAngle = (360.f / WolfCount) * i;

			FVector SpawnLocation = Avatar->GetActorLocation();

			ACirclingWolves* Wolves = GetWorld()->SpawnActor<ACirclingWolves>(
				WolvesClass, SpawnLocation, FRotator::ZeroRotator);

			if (Wolves)
			{
				Wolves->InitWolves(Avatar, StartAngle, CircleRadius, LifeTime);
			}
		}

		// 데미지 필드 소환
		if (DamageFieldClass)
		{
			ACircleDamageField* Field = GetWorld()->SpawnActor<ACircleDamageField>(
				DamageFieldClass,
				Avatar->GetActorLocation(),
				Avatar->GetActorRotation()
			);

			if (Field)
			{
				Field->AttachToActor(Avatar, FAttachmentTransformRules::KeepWorldTransform);
				Field->InitField(Avatar, CircleRadius, LifeTime);
			}
		}
	}

	CommitAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfoRef());

	if (ASC) ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
}

void UGA_Shinbi_ESkill::SpawnLensFlare()
{
	if (!GetCurrentActorInfo()->IsLocallyControlled()) return;

	AWCharacterBase* Player = Cast<AWCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Player) return;
	
	LensFlareComp = UGameplayStatics::SpawnEmitterAttached(
		LensFlareParticle,
		Player->GetFollowCamera(),
		NAME_None,
		FVector(120.f, 0.f, 0.f),
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset
	);
}

void UGA_Shinbi_ESkill::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffectClass, 1.f);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("ability.data.cooldown"), CooldownTime);

		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

void UGA_Shinbi_ESkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (LensFlareComp)
	{
		LensFlareComp->DeactivateSystem();
		LensFlareComp = nullptr;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		if (ASC)
		{
			ASC->RemoveGameplayCue(GetSpawnParticleCueTags());
		}	
	}

	if (ASC) ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
