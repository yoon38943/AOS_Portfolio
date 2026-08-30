#include "Gimmick/Projectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Tower.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"


void AProjectile::OnRep_Target()
{
	if (Target && Target->GetRootComponent())
	{
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
	}
}

void AProjectile::ApplyDamageToTarget(AActor* HitActor)
{
	if (!HitActor) return;

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(HitEffect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("ability.data.damage"), -ProjectileAttackStat);

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

AProjectile::AProjectile()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	CollisionComponent->InitSphereRadius(15);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionComponent;
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnComponentBeginOverlap);
	CollisionComponent->SetGenerateOverlapEvents(true);

	Particle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particle"));
	Particle->SetupAttachment(GetRootComponent());
	Particle->bAutoActivate = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;		// 발사체 지정(없으면 무엇을 발사할지 인식 못함)
	ProjectileMovement->InitialSpeed = 1200.f;
	ProjectileMovement->MaxSpeed = 1200.f;
	ProjectileMovement->bIsHomingProjectile = true;		// 발사체 유도 기능
	ProjectileMovement->HomingAccelerationMagnitude = 20000.f;	// 발사체 유도 민감도

	InitialLifeSpan = 5.f;	// 발사체 존재 가능 시간
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (Target)
		{
			FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
			ProjectileMovement->Activate();
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 날아가는 와중 타겟이 죽거나 사라졌을 경우
	if (HasAuthority() && (!IsValid(Target) || Target->bIsDead == true))
	{
		Target = nullptr;
		Destroy();
	}
}

void AProjectile::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (Target == OtherActor)
	{
		ApplyDamageToTarget(Target);
		Destroy();
	}
}

void AProjectile::SetHomingTarget()
{
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("Projectile has no target!"));
		return;
	}

	if (Target->GetRootComponent())
	{
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
	}
}

void AProjectile::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Target);
}
