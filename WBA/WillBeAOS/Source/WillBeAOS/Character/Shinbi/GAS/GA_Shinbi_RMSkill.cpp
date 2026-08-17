#include "Character/Shinbi/GAS/GA_Shinbi_RMSkill.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Camera/CameraComponent.h"
#include "Character/WCharacterBase.h"
#include "Character/Shinbi/DashHitCollision.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"


void UGA_Shinbi_RMSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayerChar = Cast<AWCharacterBase>(GetAvatarActorFromActorInfo());

	CurrentDashStacks = MaxDashStacks;

	PerformDash();
}

void UGA_Shinbi_RMSkill::PerformDash()
{
	CurrentDashStacks--;

	if (CurrentDashStacks > 0)
	{		
		GetWorld()->GetTimerManager().SetTimer(ReactivationTimer, [this]()
		{
			FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
			CommitAbilityCooldown(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), ActivationInfo, false);
			ApplyCooldown();
			
			EndDashAbility();
		}, ReactivationTime, false);

		StartDash();
	}
	else
	{
		FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
		CommitAbilityCooldown(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), ActivationInfo, false);
		ApplyCooldown();
		
		DashMontageEnded = true;
		StartDash();
	}
}

void UGA_Shinbi_RMSkill::StartDash()
{
	if (IInterface_CharacterAction* CharInterface = Cast<IInterface_CharacterAction>(GetAvatarActorFromActorInfo()))
	{
		CharInterface->RequestSnapToCameraDirection();
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.movement.blocked"));
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(RecastCooldownEffect, 1.f);
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("ability.data.internalcooldown"), ReactivationTime);
	ApplyGameplayEffectSpecToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfoRef(), SpecHandle);

	UGameplayStatics::SpawnEmitterAttached(
		Dash_Camera_Particle,
		PlayerChar->GetFollowCamera(),
		NAME_None,
		FVector(70.f, 0.f, 0.f),
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset
	);
	
	PlayDashAnimation();
	
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	UCapsuleComponent* Collision = Avatar->GetCapsuleComponent();
	if (Collision) Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	
	FRotator ControlRot = Avatar->GetControlRotation();
	FVector Direction = FRotator(0, ControlRot.Yaw, 0).Vector();

	Avatar->LaunchCharacter(Direction * DashSpeed, true, true);

	if (K2_HasAuthority())
	{
		ADashHitCollision* HitCollision = GetWorld()->SpawnActor<ADashHitCollision>(HitCollisionClass, Avatar->GetActorLocation(), FRotator::ZeroRotator);
		HitCollision->InitCollision(GetAvatarActorFromActorInfo(), Avatar->GetCapsuleComponent()->GetScaledCapsuleRadius(), Avatar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		HitCollision->SetLifeSpan(0.5f);
	}

	GetWorld()->GetTimerManager().SetTimer(CastingTagTimer, [this]()
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.casting"));
			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.movement.blocked"));
		}
		GetWorld()->GetTimerManager().ClearTimer(CastingTagTimer);
	}, 0.2f, false);

	GetWorld()->GetTimerManager().SetTimer(WaitingInputTimer, [this]()
	{
		OnDashLanded();
		GetWorld()->GetTimerManager().ClearTimer(WaitingInputTimer);
	}, 0.57f, false);
}

void UGA_Shinbi_RMSkill::OnDashLanded()
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return;

	UCapsuleComponent* Collision = Avatar->GetCapsuleComponent();
	if (Collision) Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	if (!DashMontageEnded)
	{
		WaitNextInput();
	}
	else
	{
		EndDashAbility();
	}
}

void UGA_Shinbi_RMSkill::PlayDashAnimation()
{
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	if (HasAuthorityOrPredictionKey(GetCurrentActorInfo(), &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, RMSkill_Dash_Montage);
		MontageTask->ReadyForActivation();
	}
}

void UGA_Shinbi_RMSkill::WaitNextInput()
{
	UAbilityTask_WaitInputPress* WaitInput = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInput->OnPress.AddDynamic(this, &ThisClass::OnNextDashInput);
	WaitInput->ReadyForActivation();
}

void UGA_Shinbi_RMSkill::OnNextDashInput(float TimeWaited)
{
	GetWorld()->GetTimerManager().ClearTimer(ReactivationTimer);

	PerformDash();
}

void UGA_Shinbi_RMSkill::ApplyCooldown()
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

void UGA_Shinbi_RMSkill::EndDashAbility()
{
	GetWorld()->GetTimerManager().ClearTimer(ReactivationTimer);
	GetWorld()->GetTimerManager().ClearTimer(CastingTagTimer);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		FGameplayTag CastingTag = FGameplayTag::RequestGameplayTag("ability.state.casting");
		int32 Count = ASC->GetTagCount(CastingTag);
		if (Count > 0)
		{
			ASC->RemoveLooseGameplayTag(CastingTag, Count);
		}
	}

	K2_EndAbility();
}
