#include "Char_Shinbi.h"

#include "Character/AOSActor.h"
#include "Kismet/KismetSystemLibrary.h"


AChar_Shinbi::AChar_Shinbi()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AChar_Shinbi::BeginPlay()
{
	Super::BeginPlay();
}

void AChar_Shinbi::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AChar_Shinbi::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	if (!HasAuthority() && IsLocallyControlled())
	{
		TArray<AActor*> TargetEnemy = GetTartgetInCenter();

		if (TargetEnemy.Num() > 0)
		{
			bIsEnemyLockOn = true;
			AttackTarget = TargetEnemy;
		}
		else
		{
			bIsEnemyLockOn = false;
			TArray<AActor*> Empty;
			AttackTarget = Empty;
		}
	}

}

TArray<AActor*> AChar_Shinbi::GetTartgetInCenter()
{
	FVector ActorLocation = GetRootComponent()->GetComponentLocation();
	FRotator ActorRotation = GetRootComponent()->GetComponentRotation();
	FVector ForwardVector = ActorRotation.Vector();

	FVector TraceStart = ForwardVector * 100 + ActorLocation;
	FVector TraceEnd = TraceStart;

	TArray<FHitResult> Hits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	ECollisionChannel EnemyChannel;
	if (GetTeamID() == E_TeamID::Blue)
	{
		EnemyChannel = CollisionInfo::RedTeam;
	}
	else
	{
		EnemyChannel = CollisionInfo::BlueTeam;
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(EnemyChannel));

	bool AttackSuccess = UKismetSystemLibrary::BoxTraceMultiForObjects(
		GetWorld(),
		TraceStart,
		TraceEnd,
		FVector(50.f, 50.f, 90.f),
		FRotator(0.f),
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		Hits,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.f
	);

	TArray<AActor*> AllTarget;

	if (AttackSuccess)
	{
		for (auto& Elem : Hits)
		{			
			if (!Cast<AAOSCharacter>(Elem.GetActor()) && !Cast<AAOSActor>(Elem.GetActor())) continue;

			AllTarget.AddUnique(Elem.GetActor());
		}

		return AllTarget;
	}

	return AllTarget;
}

void AChar_Shinbi::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}