#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "UWGameplayAbilityTypes.h"
#include "WAbilitySystemComponent.generated.h"

UCLASS()
class WILLBEAOS_API UWAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

	bool bIsGameStart = true;

public:
	void SetIsNotGameStart();
	
	void ApplyInitialStat(TObjectPtr<UDataTable> StatTable, TSubclassOf<UGameplayEffect> InitialEffect, FName ObjectName);

	void ApplyInitialEffects(TArray<TSubclassOf<UGameplayEffect>> InitialEffects);

	void GiveInitialAbilities(TMap<EWAbilityInputID, TSubclassOf<UGameplayAbility>> Abilities, TMap<EWAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities);
};
