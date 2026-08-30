#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Shinbi_RMSkill.generated.h"

class AWCharacterBase;
class ADashHitCollision;

UCLASS()
class WILLBEAOS_API UGA_Shinbi_RMSkill : public UWGameplayAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> RecastCooldownEffect;
	
	int32 MaxDashStacks = 3;
	int32 CurrentDashStacks = 0;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* RMSkill_Dash_Montage;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ADashHitCollision> HitCollisionClass;

	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* Dash_Camera_Particle;

	float ReactivationTime = 4.f;

	float DashSpeed = 7000.f;

	FTimerHandle ReactivationTimer;
	FTimerHandle CastingTagTimer;
	FTimerHandle WaitingInputTimer;

	bool DashMontageEnded = false;

	void PerformDash();

	void StartDash();

	UFUNCTION()
	void OnDashLanded();

	UFUNCTION()
	void PlayDashAnimation();

	UFUNCTION()
	void WaitNextInput();

	UFUNCTION()
	void OnNextDashInput(float TimeWaited);

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	float CooldownTime = 8.f;

	void ApplyCooldown();
	
	void EndDashAbility();
};
