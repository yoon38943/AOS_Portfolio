#include "Minions/MinionsSpawner.h"
#include "WMinionsCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "PersistentGame/PlayGameMode.h"

AMinionsSpawner::AMinionsSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AMinionsSpawner::BeginPlay()
{
	Super::BeginPlay();

	GM = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->OnGameEnd.AddUObject(this, &ThisClass::GameStateIsEnd);
	}
}

void AMinionsSpawner::GameStateIsEnd()
{
	bGameIsEnd = true;
}

void AMinionsSpawner::StartSpawnMinions()
{
	SpawnMinions();
	GetWorld()->GetTimerManager().SetTimer(InitGameTimerHandle, this, &ThisClass::SpawnMinions, 1.f, false);
}

void AMinionsSpawner::SpawnMinions_Implementation()
{
	if(InitGameTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(InitGameTimerHandle);
	}

	if (bGameIsEnd) return;
	
	SpawnCount++;

	FTransform SpawnPointTransform = FTransform(GetActorRotation(), GetActorLocation());
	AWMinionsCharacterBase* SpawnMinion = GetWorld()->SpawnActorDeferred<AWMinionsCharacterBase>(SpawnMinionsClass, SpawnPointTransform, this);
	if (SpawnMinion)
	{
		SpawnMinion->SetTeamID(TeamID);
		SpawnMinion->TrackNum = TrackNum;
		SpawnMinion->SetTrackPoint();
		UGameplayStatics::FinishSpawningActor(SpawnMinion, GetActorTransform());
		SpawnMinion->SpawnDefaultController();
	}

	if (SpawnCount < 3)
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnMinions, 1.f, false);
	}
	else
	{
		SpawnCount = 0;
		GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnMinions, 40.f, false);
	}
}

