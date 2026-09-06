#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AN_Minion_BasicAttack.generated.h"

class AWMinionsCharacterBase;

UCLASS()
class WILLBEAOS_API UAN_Minion_BasicAttack : public UAnimNotifyState
{
	GENERATED_BODY()
	
protected:
	void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;

private:
	UPROPERTY(EditAnywhere)
	FGameplayTag EventTag;
	
	UPROPERTY()
	AWMinionsCharacterBase* Minion;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	FVector PrevMidLocation;
};
