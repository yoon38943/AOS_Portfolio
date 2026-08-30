#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Wraith_BasicAttack.generated.h"

class AProjectile_Normal;
class AWCharacterBase;

UCLASS()
class WILLBEAOS_API UGA_Wraith_BasicAttack : public UWGameplayAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* BasicAttack_Montage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TSubclassOf<UGameplayEffect> Wraith_BasicAttack_Effect;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TSubclassOf<AProjectile_Normal> ProjectileClass;

	float NormalAttackDistance = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float BulletSpeed = 12000.f;

	UFUNCTION()
	void PerformAttack(FGameplayEventData Data);
	
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	void LineTraceHit(FVector TraceStart, FVector TraceEnd, FHitResult& HitResult);

	void SpawnFakeBulletCue(FVector StrikePoint);
	void SpawnHitParticle(FHitResult HitResult);

	void ServerApplyDamage(FHitResult HitResult);

	static FGameplayTag GetAttackFireEventTag();
	static FGameplayTag GetSpawnBulletCueEventTag();
};
