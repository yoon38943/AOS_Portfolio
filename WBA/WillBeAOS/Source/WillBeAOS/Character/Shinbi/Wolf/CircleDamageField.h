#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "CircleDamageField.generated.h"

class AAOSCharacter;
class UGameplayEffect;
class USphereComponent;

UCLASS()
class WILLBEAOS_API ACircleDamageField : public AActor
{
	GENERATED_BODY()
	
public:	
	ACircleDamageField();

	void InitField(AActor* InOwner, const float InRadius, const float InLifeTime);

	UPROPERTY()
	AAOSCharacter* Player;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(EditDefaultsOnly)
	USphereComponent* DamageCollision;

private:	
	UPROPERTY()
	AActor* FieldOwner;

	float FieldRadius = 350.f;

	float LifeTime = 5.f;

	UPROPERTY()
	TMap<AActor*, FTimerHandle> DamageTimers;

	UPROPERTY(EditAnywhere, Category = "Gameplay Effect")
	TSubclassOf<UGameplayEffect> Shinbi_ESkill_DamageEffect;

	UPROPERTY(EditAnywhere, Category = "Gameplay Ability")
	FGameplayTag EventDamageTag;

	void StartDamageToActor(AActor* HitActor);
	void DamageToEnemy(AActor* HitActor);

	FVector GetHitLocationForActor(AActor* HitActor) const;
};
