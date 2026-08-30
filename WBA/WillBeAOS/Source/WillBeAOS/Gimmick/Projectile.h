#pragma once

#include "CoreMinimal.h"
#include "Character/AOSCharacter.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UGameplayEffect;
class USphereComponent;

UCLASS()
class WILLBEAOS_API AProjectile : public AActor
{
	GENERATED_BODY()

	UFUNCTION()
	void OnRep_Target();

	UPROPERTY(EditAnywhere, Category = "Homing")
	float TurnSpeed = 1000.f;		// 조절 가능 회전 속도

	UPROPERTY(VisibleAnywhere, Category = "Collision")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Particle")
	UParticleSystemComponent* Particle;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> HitEffect;

	void ApplyDamageToTarget(AActor* HitActor);
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
	
	UPROPERTY(ReplicatedUsing = OnRep_Target)
	AAOSCharacter* Target;
	
	float ProjectileAttackStat;

	void SetHomingTarget();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
