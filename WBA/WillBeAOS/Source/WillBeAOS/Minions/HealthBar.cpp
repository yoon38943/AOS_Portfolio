#include "HealthBar.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"

void UHealthBar::SetAndBoundToGameplayAttribute(UAbilitySystemComponent* AbilitySystemComponent,
													  const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute)
{
	if (AbilitySystemComponent)
	{
		bool bFound;
		float Value = AbilitySystemComponent->GetGameplayAttributeValue(Attribute, bFound);
		float MaxValue = AbilitySystemComponent->GetGameplayAttributeValue(MaxAttribute, bFound);
		if (bFound)
		{
			SetValue(Value, MaxValue);
		}

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::ValueChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &ThisClass::MaxValueChanged);
	}
}

void UHealthBar::SetValue(float NewValue, float NewMaxValue)
{
	CachedValue = NewValue;
	CachedMaxValue = NewMaxValue;

	float NewPercent = NewValue / NewMaxValue;
	HealthBar->SetPercent(NewPercent);
}

void UHealthBar::ValueChanged(const FOnAttributeChangeData& Data)
{
	SetValue(Data.NewValue, CachedMaxValue);
}

void UHealthBar::MaxValueChanged(const FOnAttributeChangeData& Data)
{
	SetValue(CachedValue, Data.NewValue);
}