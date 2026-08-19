#pragma once

#include "CoreMinimal.h"
#include "Character/WCharacterBase.h"
#include "GameFramework/Actor.h"
#include "DashHitCollision.generated.h"

class UGameplayEffect;

UCLASS()
class WILLBEAOS_API ADashHitCollision : public AActor
{
	GENERATED_BODY()
	
public:	
	ADashHitCollision();

	void InitCollision(AActor* InOwner, float InRadius, float InHalfHeight);

	UPROPERTY()
	AWCharacterBase* Player;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	AActor* SkillOwner;

	UPROPERTY()
	TArray<AActor*> HitActors;

	UPROPERTY(EditAnywhere, Category = "Gameplay Effect")
	TSubclassOf<UGameplayEffect> Shinbi_RMSkill_DamageEffect;

	float Radius;
	float HalfHeight;

	bool bCanCheck = false;

	FVector PrevLocation;
	
	void CheckHitPath();
	
	void ApplyDamageToTarget(AActor* HitActor, FHitResult& HitResult);
};
