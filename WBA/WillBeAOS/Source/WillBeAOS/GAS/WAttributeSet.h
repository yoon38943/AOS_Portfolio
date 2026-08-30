#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
		GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class WILLBEAOS_API UWAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	ATTRIBUTE_ACCESSORS(UWAttributeSet, Health)	
	ATTRIBUTE_ACCESSORS(UWAttributeSet, MaxHealth)
	ATTRIBUTE_ACCESSORS(UWAttributeSet, AttackStat)
	ATTRIBUTE_ACCESSORS(UWAttributeSet, DefenseStat)
	ATTRIBUTE_ACCESSORS(UWAttributeSet, SpeedStat)
	
	virtual void GetLifetimeReplicatedProps( TArray< class FLifetimeProperty > & OutLifetimeProps ) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_AttackStat)
	FGameplayAttributeData AttackStat;

	UPROPERTY(ReplicatedUsing = OnRep_AttackStat)
	FGameplayAttributeData DefenseStat;

	UPROPERTY(ReplicatedUsing = OnRep_AttackStat)
	FGameplayAttributeData SpeedStat;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackStat(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DefenseStat(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SpeedStat(const FGameplayAttributeData& OldValue);
};
