#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_Shinbi_BasicAttackTrace.generated.h"

class AWCharacterBase;

UCLASS()
class WILLBEAOS_API UANS_Shinbi_BasicAttackTrace : public UAnimNotifyState
{
	GENERATED_BODY()
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	FVector PrevMidLocation;

	UPROPERTY()
	AWCharacterBase* Player;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	UPROPERTY(EditAnywhere, Category = "Gameplay Ability")
	FGameplayTag EventTag;
};
