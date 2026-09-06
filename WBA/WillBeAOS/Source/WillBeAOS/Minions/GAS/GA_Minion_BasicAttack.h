#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Minion_BasicAttack.generated.h"

class AWMinionsCharacterBase;

UCLASS()
class WILLBEAOS_API UGA_Minion_BasicAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
protected:
	void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> BasicAttack_DamageEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	UAnimMontage* AttackMontage;

	UPROPERTY()
	AWMinionsCharacterBase* Minion;
	
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	UFUNCTION()
	void DoDamage(FGameplayEventData Data);
};
