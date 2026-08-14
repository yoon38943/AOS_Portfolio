#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/WCharAnimInstance.h"
#include "Animation/BlendSpace.h"
#include "Wraith_AnimInstance.generated.h"

UCLASS()
class WILLBEAOS_API UWraith_AnimInstance : public UWCharAnimInstance
{
	GENERATED_BODY()
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Animation|ShootingMode")
	FGameplayTag CurrentShootingModeTag;

	virtual FGameplayTag GetCurrentShootingModeTag() const override { return CurrentShootingModeTag; }


	// AimOffset
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimOffset")
	UBlendSpace* AO_NonCombat; 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimOffset")
	UBlendSpace* AO_Combat; 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimOffset")
	UBlendSpace* AO_Snipe; 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimOffset")
	UBlendSpace* AO_Bomb;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AnimOffset", meta = (BlueprintThreadSafe))
	UBlendSpace* SelectAimOffsetByShootingTag(FGameplayTag ShootingTag) const;

	// TurnInPlace
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TurnInPlace")
	UBlendSpace* BS_Turn_NonCombat;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TurnInPlace")
	UBlendSpace* BS_Turn_Combat;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TurnInPlace")
	UBlendSpace* BS_Turn_Bomb;

	/*UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TurnInPlace", meta = (BlueprintThreadSafe))
	UBlendSpace* SelectTurnInPlaceByShootingTag(FGameplayTag ShootingTag);*/

private:
	void OnShootingModeTagCountChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY()
	class UAbilitySystemComponent* CachedASC;
};
