#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile_RMSkill.generated.h"

class UProjectileMovementComponent;

UCLASS()
class WILLBEAOS_API AProjectile_RMSkill : public AActor
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly)
	UParticleSystemComponent* ProjectileParticle;

	UPROPERTY(EditAnywhere, Category = "Particle")
	UParticleSystem* ProjectileTrail;

	UPROPERTY(EditDefaultsOnly)
	UProjectileMovementComponent* ProjectileMovement;
	
public:	
	AProjectile_RMSkill();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	FVector StartLocation;
	float HitPointDistSquared;

public:	
	float TraceLength;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	bool bIsLocallyControlled;
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FVector MuzzleLocation;
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FVector TargetPoint;
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	float BulletSpeed;
};
