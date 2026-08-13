#include "Projectile_Normal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"


AProjectile_Normal::AProjectile_Normal()
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

void AProjectile_Normal::BeginPlay()
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
}

void AProjectile_Normal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float DistToStart = FVector::DistSquared(GetActorLocation(), StartLocation);
	if (DistToStart > HitPointDistSquared)
	{
		Destroy();
	}
}
