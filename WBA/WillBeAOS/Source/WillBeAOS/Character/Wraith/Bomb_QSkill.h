#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bomb_QSkill.generated.h"

class UGameplayEffect;
enum class E_TeamID : uint8;
class URotatingMovementComponent;
class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class WILLBEAOS_API ABomb_QSkill : public AActor
{
	GENERATED_BODY()

	float BombDamage = 0.f;

protected:
	UPROPERTY(EditAnywhere)
	UParticleSystem* ExplosionEffect;

public:	
	ABomb_QSkill();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USphereComponent* CollisionComp;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* BombMesh;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	URotatingMovementComponent* RotatingComponent;

	int64 UniqueID;

	E_TeamID TeamID;
	
	float CurrentRollValue = 0.f;

	UPROPERTY(Replicated)
	FVector InitialVelocity;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> QSkill_Damage_Effect;


protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult);
	
	void Explode();
	
	void ApplyBombDamage(AActor* HitActor);

	UFUNCTION(NetMulticast, Reliable)
	void SpawnParticle();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
