#include "WCharacterHUD.h"

#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "PersistentGame/GamePlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/WAbilitySystemComponent.h"
#include "PersistentGame/PlayGameState.h"


void UWCharacterHUD::NativeConstruct()
{
	Super::NativeConstruct();

	AWGS = GetWorld()->GetGameState<APlayGameState>();
	
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		AWPS = PlayerController->GetPlayerState<AGamePlayerState>();

		if (AWPS)
		{
			auto Message = FString::Printf(TEXT("PlayerState 가져오기 성공: %s"), *AWPS->GetName());
		}
		else
		{
			// PlayerState가 NULL. 0.5초 후 재시도
			GetWorld()->GetTimerManager().SetTimer(ErrorTimerHandle, this, &UWCharacterHUD::TryGetPlayerState, 0.2f, true);
		}
	}

	UpdateCharacter();
}

void UWCharacterHUD::SetAttributeSetStatInfo(AGamePlayerState* PS)
{
	OwnerAbilitySystemComponent = PS->GetAbilitySystemComponent();
	if (OwnerAbilitySystemComponent)
	{
		SetAndBoundToGameplayAttribute(OwnerAbilitySystemComponent, UWAttributeSet::GetHealthAttribute(), UWAttributeSet::GetMaxHealthAttribute());
	}
}

void UWCharacterHUD::SetAndBoundToGameplayAttribute(UWAbilitySystemComponent* AbilitySystemComponent,
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

		// 체력값이 바뀔때마다 호출될 함수 델리게이트 바인딩
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::ValueChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &ThisClass::MaxValueChanged);
	}
}

void UWCharacterHUD::SetValue(float NewValue, float NewMaxValue)
{
	CachedValue = NewValue;
	CachedMaxValue = NewMaxValue;
	
	if (NewMaxValue == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Value Guage: %s, NewMaxValue can't be 0"), *GetName());
		return;
	}

	float NewPercent = NewValue / NewMaxValue;
	HealthBar->SetPercent(NewPercent);

	FNumberFormattingOptions FormatOps = FNumberFormattingOptions().SetMaximumFractionalDigits(0);

	CurrentHP->SetText(
		FText::Format(
			FTextFormat::FromString("{0} / {1}"),
			FText::AsNumber(NewValue, &FormatOps),
			FText::AsNumber(NewMaxValue, &FormatOps)
		)
	);
}

void UWCharacterHUD::ValueChanged(const FOnAttributeChangeData& Data)
{
	SetValue(Data.NewValue, CachedMaxValue);
}

void UWCharacterHUD::MaxValueChanged(const FOnAttributeChangeData& Data)
{
	SetValue(CachedValue, Data.NewValue);
}

void UWCharacterHUD::TryGetPlayerState()
{
	GetWorld()->GetTimerManager().ClearTimer(ErrorTimerHandle);
	
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		AWPS = PlayerController->GetPlayerState<AGamePlayerState>();

		if (AWPS)
		{
			GetWorld()->GetTimerManager().ClearTimer(ErrorTimerHandle);
			UpdateCharacter();  // UI 업데이트
		}
	}
}

void UWCharacterHUD::UpdateCharacter()
{
	// init 스탯
	SetState();
}

float UWCharacterHUD::GetHealthBarPercentage()
{
	if (!OwnerAbilitySystemComponent) return 0.f;
	
	bool bFound;
	float Health = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetHealthAttribute(), bFound);
	float MaxHealth = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound)
	{
		return 0.f;
	}
		
	return Health / MaxHealth;
}

void UWCharacterHUD::SetState()
{
	if (AWPS)
	{
		bool bFound;
		
		// 킬, 데스 출력
		FString KillString = FString::Printf(TEXT("K : %d"), AWPS->GetKillPoints());
		KillPoint->SetText(FText::FromString(KillString));
		
		FString DeathString = FString::Printf(TEXT("D : %d"), AWPS->GetDeathPoints());
		DeathPoint->SetText(FText::FromString(DeathString));
		
		// 캐릭터 스탯 출력
		float Attack = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetAttackStatAttribute(), bFound);
		if (bFound)
		{
			FString PowerString = FString::Printf(TEXT("공격력: %d"), (int)Attack);
			Power->SetText(FText::FromString(PowerString));
		}

		FString AHString = FString::Printf(TEXT("추가체력: %d"), AWPS->CAdditionalHealth);
		AdditionalHealth->SetText(FText::FromString(AHString));

		float Defense = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetDefenseStatAttribute(), bFound);
		if (bFound)
		{
			FString DefenceString = FString::Printf(TEXT("방어력: %.0f"), Defense);
			Defence->SetText(FText::FromString(DefenceString));
		}

		float SpeedStat = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetSpeedStatAttribute(), bFound);
		if (bFound)
		{
			FString SpeedString = FString::Printf(TEXT("스피드: %.2f"), SpeedStat/600.f);
			Speed->SetText(FText::FromString(SpeedString));
		}
		
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
}

void UWCharacterHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (AWPS)
	{
		SetState();
	}
}


FText UWCharacterHUD::SetGold()
{
	if (AWPS)
	{
		FString GoldString = FString::Printf(TEXT("Gold: %d"), AWPS->CGold);
		return FText::FromString(GoldString);
	}
	return FText();
}