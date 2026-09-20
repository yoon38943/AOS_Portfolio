#include "Character/Wraith/Bomb_QSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AOSCharacter.h"
#include "Char_Wraith.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/GameState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/PlayGameState.h"


ABomb_QSkill::ABomb_QSkill()
{
	//PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = CollisionComp;

	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(CollisionComp);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(RootComponent);
	ProjectileMovement->bRotationFollowsVelocity = true;

	RotatingComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingComponent"));
	RotatingComponent->RotationRate = FRotator(-720.0f, 0.0f, 720.0f);

	SetReplicates(true);
}

void ABomb_QSkill::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABomb_QSkill::OnBeginOverlap);
		CollisionComp->OnComponentHit.AddDynamic(this, &ABomb_QSkill::OnHit);
	}

	if (TeamID == E_TeamID::Blue)
	{
		CollisionComp->SetCollisionObjectType(CollisionInfo::RedTeam);
	}
	else
	{
		CollisionComp->SetCollisionObjectType(CollisionInfo::BlueTeam);
	}

	if (!InitialVelocity.IsZero() && ProjectileMovement)
	{
		ProjectileMovement->Velocity = InitialVelocity;
	}

	SetLifeSpan(10.0f);
}

void ABomb_QSkill::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this && OtherActor != GetOwner())
	{
		Explode();
	}
}

void ABomb_QSkill::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	
	if (OtherActor && OtherActor != this && OtherActor != GetOwner())
	{
		if (IGetInfoInterface* Target = Cast<IGetInfoInterface>(OtherActor))
		{
			if (Target->GetTeamID() != TeamID)
			{
				Explode();
			}
		}
		else if (OtherComp->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic)
		{
			Explode();
		}
	}
}

void ABomb_QSkill::Explode()
{
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	ECollisionChannel EnemyCollision;
	if (TeamID == E_TeamID::Blue)
	{
		EnemyCollision = CollisionInfo::RedTeam;
	}
	else
	{
		EnemyCollision = CollisionInfo::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(EnemyCollision);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(250.f),
		Params
	);

	for (auto& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		ApplyBombDamage(HitActor);
	}

	SpawnParticle();
	Destroy();
}

void ABomb_QSkill::ApplyBombDamage(AActor* HitActor)
{
	if (!HitActor) return;

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FGameplayEffectContextHandle ContextHandle;
	ContextHandle.AddInstigator(GetOwner(), this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(QSkill_Damage_Effect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		2.5f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ABomb_QSkill::SpawnParticle_Implementation()
{
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ExplosionEffect,
			GetActorLocation(),
			GetActorRotation(),
			FVector(0.6f),
			true,
			EPSCPoolMethod::AutoRelease,
			true
		);
	}
}

void ABomb_QSkill::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InitialVelocity);
}
