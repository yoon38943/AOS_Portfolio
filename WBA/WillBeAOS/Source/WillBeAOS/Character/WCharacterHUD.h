#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "GAS/WAttributeSet.h"
#include "Skill/SkillType.h"
#include "UI/SkillIconWidget.h"
#include "WCharacterHUD.generated.h"

class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCooldownSignature, FSkillUsedInfo, UsedSkillInfo);

UCLASS()
class WILLBEAOS_API UWCharacterHUD : public UUserWidget
{
	GENERATED_BODY()

public: // GAS
	void SetAttributeSetStatInfo(AGamePlayerState* PS);
	
	void SetAndBoundToGameplayAttribute(UWAbilitySystemComponent* AbilitySystemComponent, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute);

	void SetValue(float NewValue, float NewMaxValue);

	void ValueChanged(const FOnAttributeChangeData& Data);
	void MaxValueChanged(const FOnAttributeChangeData& Data);

	float CachedValue;
	float CachedMaxValue;

	UPROPERTY()
	UWAbilitySystemComponent* OwnerAbilitySystemComponent;
public:

	FTimerHandle TimerHandle;
	FTimerHandle ErrorTimerHandle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	class AWCharacterBase* AWC;//캐릭터 받아오는 함수
	UPROPERTY(BlueprintReadOnly, Category = "GameState")
	APlayGameState* AWGS;//게임스테이트
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	AGamePlayerState* AWPS;

	UFUNCTION(BlueprintCallable, Category = "Initialize")
	void UpdateCharacter();

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Health")
	TObjectPtr<UProgressBar>HealthBar;
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthBarPercentage();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GamePoint")
	UTextBlock* KillPoint;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GamePoint")
	UTextBlock* DeathPoint;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Stat")
	UTextBlock* Power;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Stat")
	UTextBlock* AdditionalHealth;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Stat")
	UTextBlock* Defence;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Stat")
	UTextBlock* Speed;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Stat")
	UTextBlock* Gold;
	UFUNCTION(Blueprintpure, meta = (BindWidget), Category = "Stat")
	FText SetGold();
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Stat")
	UTextBlock* CurrentHP;

	FOnSkillCooldownSignature OnSkillCooldownStarted; 
	
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	bool bSkillUsed;
	UPROPERTY(BlueprintReadOnly, Category = Skill)
	float QSkillCoolDownTime;
	FTimerHandle CooldownTimerHandle;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USkillIconWidget* QSkillWidget;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USkillIconWidget* ESkillWidget;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USkillIconWidget* RMSkillWidget;

public:
	void SetState();
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// 위젯 초기화 시 실행되는 함수
	virtual void NativeConstruct() override;

	void TryGetPlayerState();
};