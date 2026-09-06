#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ItemStoreWidget.generated.h"

class APlayGameState;
class AGamePlayerState;
class AGamePlayerController;

UCLASS()
class WILLBEAOS_API UItemStoreWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY()
	AGamePlayerController* PC;
	UPROPERTY()
	AGamePlayerState* PS;
	UPROPERTY()
	APlayGameState* GS;

	int32 G_Attack;
	int32 G_Health;
	int32 G_Defence;
	int32 G_Speed;

protected:

	UPROPERTY(meta = (BindWidget))
	UButton* AddAttack;
	UPROPERTY(meta = (BindWidget))
	UButton* AddHealth;
	UPROPERTY(meta = (BindWidget))
	UButton* AddDefence;
	UPROPERTY(meta = (BindWidget))
	UButton* AddSpeed;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AttackGold;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthGold;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefenceGold;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SpeedGold;
	
	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void AddPowerState();
	
	UFUNCTION(BlueprintCallable)
	void AddHealthState();

	UFUNCTION(BlueprintCallable)
	void AddDefenceState();

	UFUNCTION(BlueprintCallable)
	void AddSpeedState();
};
