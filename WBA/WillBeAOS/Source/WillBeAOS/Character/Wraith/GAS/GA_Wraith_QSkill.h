#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Wraith_QSkill.generated.h"

class ABomb_QSkill;

UCLASS()
class WILLBEAOS_API UGA_Wraith_QSkill : public UWGameplayAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	
private:
	UPROPERTY()
	FActiveGameplayEffectHandle EffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> Wraith_QSkill_Effect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	float CooldownTime = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	UAnimMontage* StartBombModeMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	UAnimMontage* FireBombMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<ABomb_QSkill> QSkill_Bomb_Class;

	float ProjectileLaunchSpeed = 800.f;

	float BombAimingDistance = 900.f; 

	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	void PerformBombAttack();

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

	void SpawnQSkillBomb(FVector TraceStart, FVector TraceEnd);

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;;
};
