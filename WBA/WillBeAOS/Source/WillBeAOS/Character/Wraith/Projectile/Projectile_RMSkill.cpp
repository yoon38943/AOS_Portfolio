#include "Projectile_RMSkill.h"
#include "../Char_Wraith.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"


AProjectile_RMSkill::AProjectile_RMSkill()
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileParticle = CreateDefaultSubobject<UParticleSystemComponent>("ProjectileParticle");
	RootComponent = ProjectileParticle;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 0.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	SetLifeSpan(5.f);
}

void AProjectile_RMSkill::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();
	HitPointDistSquared = FVector::DistSquared(StartLocation, TargetPoint);

	FVector Direction = (TargetPoint - GetActorLocation()).GetSafeNormal();
	SetActorRotation(Direction.Rotation());
	ProjectileMovement->Velocity = Direction * BulletSpeed;
	ProjectileMovement->MaxSpeed = BulletSpeed;

	if (bIsLocallyControlled)
	{
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetComponentTickEnabled(false);

		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			if (IsValid(this) && ProjectileMovement)
			{
				ProjectileMovement->Activate();
				ProjectileMovement->SetComponentTickEnabled(true);

				FVector Direction = (TargetPoint - GetActorLocation()).GetSafeNormal();
				ProjectileMovement->Velocity = Direction * BulletSpeed;
			}
		});
	}
	
	if (ProjectileTrail)
	{
		float Distance = FVector::Dist(TargetPoint, MuzzleLocation);
		FRotator SpawnRotator = (TargetPoint - MuzzleLocation).Rotation();
	
		UParticleSystemComponent* SpawnTrail = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ProjectileTrail,
			TargetPoint,
			SpawnRotator
		);

		if (SpawnTrail)
		{
			float ScaleX = Distance / 100.f;
			SpawnTrail->SetRelativeScale3D(FVector(-ScaleX, 1.5f, 1.5f));
		}
	}
}

void AProjectile_RMSkill::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float DistanceFromPlayer = FVector::DistSquared(GetActorLocation(), StartLocation);
	if (DistanceFromPlayer > HitPointDistSquared)
	{
		Destroy();
	}
}