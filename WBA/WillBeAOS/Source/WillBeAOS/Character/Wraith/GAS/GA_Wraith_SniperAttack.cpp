#include "Character/Wraith/GAS/GA_Wraith_SniperAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Character/WCharacterBase.h"
#include "Character/Wraith/Projectile/BulletTargetActor.h"
#include "Character/Wraith/Projectile/FBulletTargetData.h"
#include "GAS/WAbilitySystemComponent.h"


void UGA_Wraith_SniperAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                              const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, 1.f, NAME_None, false);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->ReadyForActivation();

		FGameplayTagContainer AimingTags;
		AimingTags.AddTag(FGameplayTag::RequestGameplayTag(FName("ability.state.Aiming")));
		ASC->CancelAbilities(&AimingTags, nullptr, this);
	}
	
	PerformAttack();
}

void UGA_Wraith_SniperAttack::PerformAttack()
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
				BulletActor->AttackDistance = SniperAttackDistance;
			}
			TargetDataTask->FinishSpawningActor(this, SpawnedActor);
		}
		TargetDataTask->ReadyForActivation();
	}
}

void UGA_Wraith_SniperAttack::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
	if (Data.Num() == 0) return;
	
	const FBulletTargetData* BulletData = static_cast<const FBulletTargetData*>(Data.Get(0));
	if (!BulletData) return;
	
	FVector TraceStart = BulletData->TraceStart;
	FVector TraceEnd = BulletData->TraceEnd;

	FHitResult HitResult;
	LineTraceHit(TraceStart, TraceEnd, HitResult);

	bool bHit = HitResult.bBlockingHit;
	FVector StrikePoint = bHit ? HitResult.ImpactPoint : TraceEnd;

	if (HasAuthorityOrPredictionKey(GetCurrentActorInfo(), &GetCurrentActivationInfoRef()))
	{
		SpawnFakeBulletCue(StrikePoint);
		SpawnHitParticle(HitResult);
	}
	
	if (K2_HasAuthority())
	{
		ServerApplyDamage(HitResult);
	}

	CommitAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfoRef());
}

void UGA_Wraith_SniperAttack::LineTraceHit(FVector TraceStart, FVector TraceEnd, FHitResult& HitResult)
{	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	ECollisionChannel EnemyChannel;
	if (Avatar->GetTeamID() == E_TeamID::Blue)
		EnemyChannel = CollisionInfo::RedTeam;
	else
		EnemyChannel = CollisionInfo::BlueTeam;
	
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(EnemyChannel);

	GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		TraceStart,
		TraceEnd,
		ObjectQuery,
		QueryParams
	);
}

void UGA_Wraith_SniperAttack::SpawnFakeBulletCue(FVector StrikePoint)
{
	FVector MuzzleLoc = Avatar->GetMesh()->GetSocketLocation("Muzzle_01");
	
	FGameplayEffectContextHandle BulletContextHandle = FGameplayEffectContextHandle(new FBulletContext);

	FBulletContext* BulletContext = static_cast<FBulletContext*>(BulletContextHandle.Get());
	if (!BulletContext) return;
	
	BulletContext->MuzzleLoc = MuzzleLoc;
	BulletContext->StrikePoint = StrikePoint;
	BulletContext->Speed = BulletSpeed;
	
	FGameplayCueParameters CueParams;
	CueParams.EffectContext = BulletContextHandle;

	ASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.wraith.rmskill.fire"), CueParams);
}

void UGA_Wraith_SniperAttack::SpawnHitParticle(FHitResult HitResult)
{
	if (!ASC) return;
	
	FGameplayCueParameters CueParams;
	
	FGameplayEffectContextHandle ParticleEffectContext = ASC->MakeEffectContext();
	ParticleEffectContext.AddHitResult(HitResult);
	
	CueParams.EffectContext = ParticleEffectContext;
	
	ASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.hit.wraith.sniperattack")), CueParams);
}

void UGA_Wraith_SniperAttack::ServerApplyDamage(FHitResult HitResult)
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitResult.GetActor());
	if (!TargetASC) return;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(Avatar, Avatar);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Wraith_SniperAttack_Effect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		2.5f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void UGA_Wraith_SniperAttack::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffectClass, 1.f);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("ability.data.cooldown"), CooldownTime);

		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}