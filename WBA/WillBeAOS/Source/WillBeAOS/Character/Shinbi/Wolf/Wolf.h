#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Game/WGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Wolf.generated.h"

UCLASS()
class WILLBEAOS_API AWolf : public ACharacter
{
	GENERATED_BODY()

public:
	AWolf();

	void LaunchWolf(AWCharacterBase* InInstigator);


protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UParticleSystemComponent* TrailEffect;
	
private:
	float DashSpeed = 1300.f;
	float DashEndDistance = 1200.f;

	float TotalMoveDistance;
	
	float ExplosionRadius = 150.f;

	float DashDamage = 30.f;

	float ExplosionDamage = 50.f;

	UPROPERTY()
	AWCharacterBase* WolfInstigator; 

	TArray<TWeakObjectPtr<AActor>> HitActors;

	FVector PrevLocation;

	void DashToForward(float DeltaTime);

	UPROPERTY(EditAnywhere)
	UParticleSystem* ExplodeParticle;

	void Explosion(const FVector& ImpactLocation);

	UFUNCTION(NetMulticast, Reliable)
	void Explode_Particle_Multicast(const FVector& ImpactLocation);

	UFUNCTION(NetMulticast, Reliable)
	void Disappear();

	bool bIsDisappear = false;
	
	void CheckPathHit();

	UPROPERTY(EditAnywhere, Category = "Gameplay Effect")
	TSubclassOf<UGameplayEffect> Shinbi_QSkill_DamageEffect;
	
	void ApplyDamageToTarget(AActor* HitActor, FHitResult& HitResult, bool bExplosion);
};
