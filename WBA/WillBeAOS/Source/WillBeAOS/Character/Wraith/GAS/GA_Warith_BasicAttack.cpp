#include "Character/Wraith/GAS/GA_Warith_BasicAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/WCharacterBase.h"
#include "Character/Wraith/Projectile/BulletTargetActor.h"
#include "Character/Wraith/Projectile/FBulletTargetData.h"


void UGA_Warith_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	ASC = GetAbilitySystemComponentFromActorInfo();
	Avatar = Cast<AWCharacterBase>(GetAvatarActorFromActorInfo());

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, BasicAttack_Montage);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->ReadyForActivation();
		
		UAbilityTask_WaitGameplayEvent* AttackEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetAttackFireEventTag());
		AttackEvent->EventReceived.AddDynamic(this, &ThisClass::PerformAttack);
		AttackEvent->ReadyForActivation();
	}
}

void UGA_Warith_BasicAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Warith_BasicAttack::PerformAttack(FGameplayEventData Data)
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
			TargetDataTask->FinishSpawningActor(this, SpawnedActor);
		}
		TargetDataTask->ReadyForActivation();
	}
}

void UGA_Warith_BasicAttack::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
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
	}
	
	if (K2_HasAuthority())
	{
		ServerApplyDamage(HitResult);
	}
}

void UGA_Warith_BasicAttack::LineTraceHit(FVector TraceStart, FVector TraceEnd, FHitResult& HitResult)
{	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	ECollisionChannel EnemyChannel;
	if (Avatar->GetTeamID() == E_TeamID::Blue)
		EnemyChannel = TeamCollision::RedTeam;
	else
		EnemyChannel = TeamCollision::BlueTeam;
	
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

void UGA_Warith_BasicAttack::SpawnFakeBulletCue(FVector StrikePoint)
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

	ASC->ExecuteGameplayCue(GetSpawnBulletCueEventTag(), CueParams);
}

void UGA_Warith_BasicAttack::ServerApplyDamage(FHitResult HitResult)
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitResult.GetActor());
	if (!TargetASC) return;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Wraith_BasicAttack_Effect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		1.f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

FGameplayTag UGA_Warith_BasicAttack::GetAttackFireEventTag()
{
	return FGameplayTag::RequestGameplayTag("ability.wraith.basicattack");
}

FGameplayTag UGA_Warith_BasicAttack::GetSpawnBulletCueEventTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.wraith.basicattack.fire");
}
