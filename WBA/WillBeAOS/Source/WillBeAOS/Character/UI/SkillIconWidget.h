#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Character/Skill/SkillType.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "SkillIconWidget.generated.h"

class AGamePlayerState;
class UWAbilitySystemComponent;

UCLASS()
class WILLBEAOS_API USkillIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	AGamePlayerState* PlayerState;

	UFUNCTION()
	void InitializeASC();

	UFUNCTION()
	void Handle_SkillUsed(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void Handle_InternalSkillUsed(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CooldownText;

	UPROPERTY(EditAnywhere, Category = "Skill")
	ESkillSlot SkillID;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* CooldownProgress;

	UPROPERTY(meta = (BindWidget))
	UImage* SkillIcon;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* SkillButton;

	void LoadSkillIcon();

private:
	UPROPERTY()
	UWAbilitySystemComponent* OwnerASC;

	UPROPERTY(EditInstanceOnly, Category = "Cooldown")
	FGameplayTag CooldownTag;

	UPROPERTY(EditInstanceOnly, Category = "Cooldown")
	FGameplayTag InternalCooldownTag;

	UPROPERTY(EditInstanceOnly, Category = "Cooldown")
	FLinearColor CooldownColor;

	UPROPERTY(EditInstanceOnly, Category = "Cooldown")
	FLinearColor InternalCooldownColor;
	
	bool bIsCoolingDown = false;
	bool bIsInternal = false;

	float CooldownTime = 0.0f;
	
	float CooldownTimeEnd = 0.0f;

	void CooldownUpdate(bool Isinternal);
};
