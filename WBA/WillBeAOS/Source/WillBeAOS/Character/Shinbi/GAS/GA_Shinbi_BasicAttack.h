#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_Shinbi_BasicAttack.generated.h"

UCLASS()
class WILLBEAOS_API UGA_Shinbi_BasicAttack : public UWGameplayAbility
{
	GENERATED_BODY()
	
public:
	void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	static FGameplayTag GetComboChangeEventTag();
	static FGameplayTag GetComboChangeEventEndTag();
	static FGameplayTag GetComboTargetEventTag();
	
private:
	void SetupWaitComboInputPress();

	UFUNCTION()
	void HandleInputPress(float TimeWaited);

	void TryCommitCombo();
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ComboMontage;

	UFUNCTION()
	void ComboChangedEventReceived(FGameplayEventData Data);

	UFUNCTION()
	void DoDamage(FGameplayEventData Data);

	FName NextComboName;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	bool bComboInputEnabled = false;

	UPROPERTY(EditAnywhere, Category = "Gameplay Effect")
	TSubclassOf<UGameplayEffect> BasicAttack_DamageEffect;
};
