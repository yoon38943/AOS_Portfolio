#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBar.generated.h"


struct FGameplayAttribute;
struct FOnAttributeChangeData;
class UAbilitySystemComponent;

UCLASS()
class WILLBEAOS_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UProgressBar>HealthBar;

	float CachedValue;
	float CachedMaxValue;

	void SetAndBoundToGameplayAttribute(UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute);

	void SetValue(float NewValue, float NewMaxValue);

	void ValueChanged(const FOnAttributeChangeData& Data);
	void MaxValueChanged(const FOnAttributeChangeData& Data);
};
