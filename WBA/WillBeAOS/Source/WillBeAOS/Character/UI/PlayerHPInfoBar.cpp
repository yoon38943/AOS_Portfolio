#include "Character/UI/PlayerHPInfoBar.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GAS/WAttributeSet.h"


void UPlayerHPInfoBar::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPlayerHPInfoBar::NativePreConstruct()
{
	Super::NativeConstruct();

	HealthBarSize->SetWidthOverride(BaseWidthSize);
	HealthBarSize->SetHeightOverride(BaseHeightSize);
}

void UPlayerHPInfoBar::SetAndBoundToGameplayAttribute(UAbilitySystemComponent* AbilitySystemComponent,
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

void UPlayerHPInfoBar::SetValue(float NewValue, float NewMaxValue)
{
	CachedValue = NewValue;
	CachedMaxValue = NewMaxValue;

	float NewPercent = NewValue / NewMaxValue;
	PlayerHPBar->SetPercent(NewPercent);
}

void UPlayerHPInfoBar::ValueChanged(const FOnAttributeChangeData& Data)
{
	SetValue(Data.NewValue, CachedMaxValue);
}

void UPlayerHPInfoBar::MaxValueChanged(const FOnAttributeChangeData& Data)
{
	SetValue(CachedValue, Data.NewValue);
}

void UPlayerHPInfoBar::SetBarScale(float Scale)
{
	if (!HealthBarSize) return;
	if (!PlayerNickName) return;

	float TextScale = Scale * 1.2f;
	TextScale = TextScale > 1.f ? 1.f : TextScale;
	
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(TextScale, TextScale);
	PlayerNickName->SetRenderTransform(Transform);
	
	HealthBarSize->SetWidthOverride(BaseWidthSize * Scale);
}
