#pragma once

#include "CoreMinimal.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WGameplayAbility.h"
#include "GA_All_Character_Recall.generated.h"

class URecallWidget;
class AWCharacterBase;

UCLASS()
class WILLBEAOS_API UGA_All_Character_Recall : public UWGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	float RecallTime = 7.7f;
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	FGameplayTag RecallCueTag;
	
	UPROPERTY()
	UAnimMontage* RecallMontage;

	UPROPERTY()
	UAnimMontage* CompleteRecallMontage;

	FTimerHandle RecallTimerHandle;
	
	void CallRecall_Server();
	void CallRecall_Client();
	void CompleteRecall();
	void RecallToBase();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> RecallWidgetClass;

	UPROPERTY()
	URecallWidget* RecallWidget;

	void ShowRecallWidget();
	void HideRecallWidget();

	UFUNCTION()
	void OnRecallPressAgain(float TimeElapsed);
};
