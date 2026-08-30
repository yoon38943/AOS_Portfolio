#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "BulletTargetActor.generated.h"

class AWCharacterBase;

UCLASS()
class WILLBEAOS_API ABulletTargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	ABulletTargetActor();
	
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;

	UPROPERTY()
	AWCharacterBase* Avatar;
	
	float AttackDistance = 0.f;
};
