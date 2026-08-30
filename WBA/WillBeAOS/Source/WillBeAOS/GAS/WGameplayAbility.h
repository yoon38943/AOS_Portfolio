#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WGameplayAbility.generated.h"

class AWCharacterBase;
class UWAbilitySystemComponent;

UCLASS()
class WILLBEAOS_API UWGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TSubclassOf<UGameplayEffect> CombatEffectClass;

	UPROPERTY()
	TObjectPtr<UWAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<AWCharacterBase> Avatar;
	
	class UAnimInstance* GetOwnerAnimInstance() const;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
