#include "Character/Wraith/GAS/GA_Wraith_QSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Character/WCharacterBase.h"
#include "Character/Wraith/Bomb_QSkill.h"
#include "Character/Wraith/Char_Wraith.h"
#include "Character/Wraith/Projectile/BulletTargetActor.h"
#include "Character/Wraith/Projectile/FBulletTargetData.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


void UGA_Wraith_QSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CanActivateAbility(Handle, ActorInfo))
	{
		K2_EndAbility();
		return;
	}

	if (Avatar)
	{
		AChar_Wraith* Wraith = Cast<AChar_Wraith>(Avatar);
		if (Wraith)
		{
			Wraith->bIsQSkillUsing = true;
		}
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
		if (Spec && !Spec->InputPressed)
		{
			OnInputReleased(0.f);	// 키를 즉시 떼는 경우
		}
		else
		{
			UAbilityTask_PlayMontageAndWait* StartBombMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, StartBombModeMontage);
			StartBombMontageTask->ReadyForActivation();
		
			UAbilityTask_WaitInputRelease* WaitReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
			WaitReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
			WaitReleaseTask->ReadyForActivation();
		}
	}

	if (K2_HasAuthority())
	{
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
		if (!SourceASC) return;

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Wraith_QSkill_Effect, 1.f, EffectContext);
		if (!SpecHandle.IsValid()) return;

		EffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UGA_Wraith_QSkill::OnInputReleased(float TimeHeld)
{
	if (K2_HasAuthority())
	{
		if (!EffectHandle.IsValid()) return;

		BP_RemoveGameplayEffectFromOwnerWithHandle(EffectHandle);
	}

	if (Avatar)
	{
		AChar_Wraith* Wraith = Cast<AChar_Wraith>(Avatar);
		if (Wraith)
		{
			Wraith->bIsQSkillUsing = false;
			Wraith->ClearTrajectoryPath();
		}
	}
	
	if (HasAuthorityOrPredictionKey(GetCurrentActorInfo(), &GetCurrentActivationInfoRef()))
	{
		UAbilityTask_PlayMontageAndWait* PlayQSkillMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireBombMontage);
		PlayQSkillMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
		PlayQSkillMontageTask->ReadyForActivation();

		PerformBombAttack();
	}
}

void UGA_Wraith_QSkill::PerformBombAttack()
{
	if (IsLocallyControlled() || K2_HasAuthority())
	{		
		UAbilityTask_WaitTargetData* TargetDataTask = UAbilityTask_WaitTargetData::WaitTargetData(
			this, NAME_None,
			EGameplayTargetingConfirmation::Instant,
			ABulletTargetActor::StaticClass());
		TargetDataTask->ValidData.AddDynamic(this, &ThisClass::OnTargetDataReady);
		
		AGameplayAbilityTargetActor* SpawnedActor = nullptr;
		if (TargetDataTask->BeginSpawningActor(this, ABulletTargetActor::StaticClass(), SpawnedActor))
		{
			// 스폰된 액터에 값 설정하고 싶으면 여기서
			ABulletTargetActor* BulletActor = Cast<ABulletTargetActor>(SpawnedActor);
			if (BulletActor)
			{
				BulletActor->AttackDistance = BombAimingDistance;
			}
			TargetDataTask->FinishSpawningActor(this, SpawnedActor);
		}
		TargetDataTask->ReadyForActivation();
	}
}

void UGA_Wraith_QSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	if (Data.Num() == 0) return;
	
	const FBulletTargetData* BulletData = static_cast<const FBulletTargetData*>(Data.Get(0));
	if (!BulletData) return;
	
	FVector TraceStart = BulletData->TraceStart;
	FVector TraceEnd = BulletData->TraceEnd;

	if (K2_HasAuthority())
		SpawnQSkillBomb(TraceStart, TraceEnd);
	
	ApplyCooldown();
}

void UGA_Wraith_QSkill::SpawnQSkillBomb(FVector TraceStart, FVector TraceEnd)
{
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	FVector TargetLocation;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);
	
	if (bHit)
	{
		TargetLocation = HitResult.ImpactPoint;
	}
	else
	{
		TargetLocation = TraceEnd;
	}

	FVector StartLocation = Avatar->GetMesh()->GetSocketLocation(TEXT("Muzzle_03"));
	FVector OutLaunchVelocity;
	bool bHaveSolution = false;

	float Distance = FVector::Dist(StartLocation, TargetLocation);

	float MinDist = 500.f;
	float MaxDist = 1500.f;
	float MinSpeed = ProjectileLaunchSpeed;
	float MaxSpeed = ProjectileLaunchSpeed * 2.5f;

	float Alpha = FMath::Clamp((Distance - MinDist) / (MaxDist - MinDist), 0.f, 1.f);
	float CurrentSpeed = FMath::Lerp(MinSpeed, MaxSpeed, Alpha);
	
	bHaveSolution = UGameplayStatics::SuggestProjectileVelocity(
		Avatar,
		OutLaunchVelocity,
		StartLocation,
		TargetLocation,
		CurrentSpeed,
		false, 0.f, 0.f,
		ESuggestProjVelocityTraceOption::DoNotTrace
	);
	
	if (bHaveSolution)
	{
		FTransform SpawnTransform(OutLaunchVelocity.Rotation(), StartLocation);
		ABomb_QSkill* Bomb = GetWorld()->SpawnActorDeferred<ABomb_QSkill>(QSkill_Bomb_Class, SpawnTransform);
		
		if(Bomb)
		{			
			Bomb->SetOwner(Avatar);
			Bomb->TeamID = Avatar->GetTeamID();
			Bomb->ProjectileMovement->Velocity = OutLaunchVelocity;
			Bomb->InitialVelocity = OutLaunchVelocity;
			
			float LaunchSpeed = OutLaunchVelocity.Size();
			Bomb->ProjectileMovement->MaxSpeed = LaunchSpeed;

			Bomb->CollisionComp->IgnoreActorWhenMoving(Avatar, true);
			Avatar->MoveIgnoreActorAdd(Bomb);

			UGameplayStatics::FinishSpawningActor(Bomb, SpawnTransform);
		}
	}
	else
	{
		FVector LookDir = (TargetLocation - StartLocation).GetSafeNormal();
		FTransform SpawnTransform(LookDir.Rotation(), StartLocation);
		ABomb_QSkill* Bomb = GetWorld()->SpawnActorDeferred<ABomb_QSkill>(QSkill_Bomb_Class, SpawnTransform);
		
		if(Bomb)
		{
			Bomb->SetOwner(Avatar);
			Bomb->TeamID = Avatar->GetTeamID();
			
			// LookDir은 단위벡터이므로 원하는 발사 속도로 곱해서 실제 속도를 설정
			float LaunchSpeed = FMath::Max(ProjectileLaunchSpeed, 100.f); // 최소 속도 보장
			FVector LaunchVelocity = LookDir * LaunchSpeed;

			Bomb->ProjectileMovement->Velocity = LaunchVelocity;
			Bomb->ProjectileMovement->MaxSpeed = LaunchSpeed;
			Bomb->InitialVelocity = OutLaunchVelocity;

			Bomb->CollisionComp->IgnoreActorWhenMoving(Avatar, true);
			Avatar->MoveIgnoreActorAdd(Bomb);

			UGameplayStatics::FinishSpawningActor(Bomb, SpawnTransform);
		}
	}
}

void UGA_Wraith_QSkill::ApplyCooldown()
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

void UGA_Wraith_QSkill::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateCancelAbility)
{
	if (K2_HasAuthority())
	{
		if (!EffectHandle.IsValid()) return;

		BP_RemoveGameplayEffectFromOwnerWithHandle(EffectHandle);
	}
	
	if (Avatar)
	{
		AChar_Wraith* Wraith = Cast<AChar_Wraith>(Avatar);
		if (Wraith)
		{
			Wraith->bIsQSkillUsing = false;
			Wraith->ClearTrajectoryPath();
		}
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}
