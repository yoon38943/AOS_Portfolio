#include "GAS/WAbilitySystemComponent.h"

#include "StatDataTable.h"


void UWAbilitySystemComponent::SetIsNotStartGame()
{
	bIsStartGame = false;
}

void UWAbilitySystemComponent::ApplyInitialStat(TObjectPtr<UDataTable> StatTable, TSubclassOf<UGameplayEffect> InitialEffect, FName ObjectName)
{
	if (!bIsStartGame) return;
	
	if (!StatTable) return;

	FStatDataTable* StatRow = StatTable->FindRow<FStatDataTable>(ObjectName, TEXT("InitStat"));
	if (!StatRow) return;

	if (!InitialEffect) return;

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(InitialEffect, 1, ContextHandle);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Health")), StatRow->Health_Stat);
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Attack")), StatRow->Attack_Stat);
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Defense")), StatRow->Defense_Stat);
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Speed")), StatRow->Speed_Stat);

		ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UWAbilitySystemComponent::ApplyInitialEffects(TArray<TSubclassOf<UGameplayEffect>> InitialEffects)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())	return;
	
	for (const TSubclassOf<UGameplayEffect>& EffectClass : InitialEffects)
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(EffectClass, 1, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

void UWAbilitySystemComponent::GiveInitialAbilities(TMap<EWAbilityInputID, TSubclassOf<UGameplayAbility>> Abilities, TMap<EWAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities)
{
	if (!bIsStartGame) return;
	
	if (!GetOwner() || !GetOwner()->HasAuthority())	return;

	for (const TPair<EWAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : Abilities)
	{
		GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 0, (int32)AbilityPair.Key, nullptr));
	}

	for (const TPair<EWAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : BasicAbilities)
	{
		GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 1, (int32)AbilityPair.Key, this));
	}
}
