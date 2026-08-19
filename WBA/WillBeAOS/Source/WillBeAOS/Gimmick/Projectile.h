#pragma once

#include "CoreMinimal.h"
#include "Character/AOSCharacter.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;

UCLASS()
class WILLBEAOS_API AProjectile : public AActor
{
	GENERATED_BODY()

	UPROPERTY(ReplicatedUsing = OnRep_Target)
	AAOSCharacter* Target;

	UFUNCTION()
	void OnRep_Target();

	UPROPERTY(EditAnywhere, Category = "Homing")
	float TurnSpeed = 1000.f;		// 조절 가능 회전 속도

	UPROPERTY(VisibleAnywhere, Category = "Collision")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Particle")
	class UParticleSystemComponent* Particle;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	class UProjectileMovementComponent* ProjectileMovement;
	
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

	void SetHomingTarget();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
