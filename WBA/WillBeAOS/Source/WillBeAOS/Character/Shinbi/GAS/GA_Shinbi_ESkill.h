#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Shinbi_ESkill.generated.h"


class ACircleDamageField;
class ACirclingWolves;

UCLASS()
class WILLBEAOS_API UGA_Shinbi_ESkill : public UWGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TSubclassOf<ACirclingWolves> WolvesClass;
	
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* ESKill_SpawnCircleWolves_Montage;

	UPROPERTY()
	UParticleSystemComponent* SpawnParticle;

	float CircleRadius = 350.f;

	float LifeTime = 5.f;

	static FGameplayTag GetSpawnParticleCueTags();
	static FGameplayTag GetSpawnCircleWolvesTags();

	UFUNCTION()
	void SpawnCircleWolves(FGameplayEventData Data);

	UPROPERTY()
	UParticleSystemComponent* LensFlareComp;

	UPROPERTY(EditDefaultsOnly, Category = "Particle")
	UParticleSystem* LensFlareParticle;
	
	void SpawnLensFlare();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ACircleDamageField> DamageFieldClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	float CooldownTime = 12.f;
	
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
};
