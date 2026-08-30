#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Wraith_RMSkill.generated.h"


UCLASS()
class WILLBEAOS_API UGA_Wraith_RMSkill : public UWGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override; 	
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	
private:
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* StartSnipeModeMontage;
	
	UPROPERTY()
	FActiveGameplayEffectHandle EffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> Wraith_RMSkill_Effect;

	UFUNCTION()
	void OnCancelRMSkillInput(float TimeHeld);
};
