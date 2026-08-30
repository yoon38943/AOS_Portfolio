#include "Nexus.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "../Character/CombatComponent.h"
#include "Components/SphereComponent.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "PersistentGame/PlayGameMode.h"
#include "PersistentGame/PlayGameState.h"

ANexus::ANexus()
{
	PrimaryActorTick.bCanEverTick = true;

	FakeRootCollision = CreateDefaultSubobject<USphereComponent>("FakeRootCollision");
	SetRootComponent(FakeRootCollision);
	
	NexusMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NexusMesh"));
	NexusMeshComponent->SetupAttachment(GetRootComponent());

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	EndingCameraComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EndingCamera"));
	EndingCameraComponent->SetupAttachment(GetRootComponent());

	WAbilitySystemComponent = CreateDefaultSubobject<UWAbilitySystemComponent>(TEXT("ASC"));
	WAttributeSet = CreateDefaultSubobject<UWAttributeSet>(TEXT("AttributeSet"));
	
	bReplicates = true;
	bAlwaysRelevant = true;

	NexusMeshComponent->SetReceivesDecals(false);
}

void ANexus::SetTeamCollision()
{
	if (GetTeamID() == E_TeamID::Blue)
	{
		NexusMeshComponent->SetCollisionObjectType(TeamCollision::BlueTeam);
	}
	if (GetTeamID() == E_TeamID::Red)
	{
		NexusMeshComponent->SetCollisionObjectType(TeamCollision::RedTeam);
	}
}

void ANexus::DestroyNexus()
{
	NM_DestroyNexus();
}

UAbilitySystemComponent* ANexus::GetAbilitySystemComponent() const
{
	return WAbilitySystemComponent;
}

void ANexus::NM_DestroyNexus_Implementation()
{
	FVector DestroypParticleLocation = NexusMeshComponent->GetComponentLocation();
	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		DestroyParticle,
		FVector(DestroypParticleLocation.X, DestroypParticleLocation.Y, 400),
		FRotator::ZeroRotator,
		FVector(1.f),
		true);

	AActor* EndingCamera = GetWorld()->SpawnActor<AActor>(EndingCameraClass, EndingCameraComponent->GetComponentTransform(), FActorSpawnParameters());
	
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController && EndingCamera)
	{
		PlayerController->SetViewTargetWithBlend(EndingCamera, 1.f, EViewTargetBlendFunction::VTBlend_Linear, 0.0f, false);
	}

	FTimerHandle DestroyTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle,
		[this]()
		{
			Destroy();
		},
		1.5f,
		false);
}

void ANexus::BeginPlay()
{
	Super::BeginPlay();

	WAbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->GameManagedActors.AddUnique(this);
		}

		WAbilitySystemComponent->ApplyInitialStat(StatTable, InitStatEffect, ActorName);
		WAbilitySystemComponent->ApplyInitialEffects(InitialEffects);
	}

	SetTeamCollision();
}

void ANexus::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
	if (GS)
	{
		GS->GameManagedActors.Remove(this);
	}
}

float ANexus::GetNexusHPPercent()
{
	bool bFound;
	float Health = WAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetHealthAttribute(), bFound);
	float MaxHealth = WAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetMaxHealthAttribute(), bFound);
	return Health / MaxHealth;
}
