#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Wraith_SniperAttack.generated.h"

class AWCharacterBase;

UCLASS()
class WILLBEAOS_API UGA_Wraith_SniperAttack : public UWGameplayAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> Wraith_SniperAttack_Effect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	float CooldownTime = 8.f;

	float SniperAttackDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float BulletSpeed = 12000.f;

	void PerformAttack();

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);
	void LineTraceHit(FVector TraceStart, FVector TraceEnd, FHitResult& HitResult);
	void SpawnFakeBulletCue(FVector StrikePoint);
	void SpawnHitParticle(FHitResult HitResult);
	void ServerApplyDamage(FHitResult HitResult);
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;;
};
