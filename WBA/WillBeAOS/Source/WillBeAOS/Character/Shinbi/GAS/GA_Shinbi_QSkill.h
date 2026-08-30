#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Shinbi_QSkill.generated.h"

class AWCharacterBase;
class UAbilityTask_PlayMontageAndWait;
class AWolf;

UCLASS()
class WILLBEAOS_API UGA_Shinbi_QSkill : public UWGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;

private:
	UFUNCTION()
	void SpawnDashWolf(FGameplayEventData Data);

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TSubclassOf<AWolf> WolfClass;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* QSkill_Keydown_Montage;

	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* KeydownTask;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* QSkill_SpawnDashWolf_Montage;

	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	static FGameplayTag GetQSkillSpawnWolfEventTag();

	UPROPERTY()
	UDecalComponent* SpawnedDecal;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* SkillRangeDecalMaterial;
	
	void SpawnDashRangeDecal();

	/*************************************************/
	// 쿨타임
	/*************************************************/

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	float CooldownTime = 8.f;  // 나중에 레벨별 쿨다운으로 변경해보기

	void ApplyCooldown();
};
