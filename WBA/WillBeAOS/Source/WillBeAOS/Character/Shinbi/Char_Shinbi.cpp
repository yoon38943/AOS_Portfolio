#include "Char_Shinbi.h"

#include "Character/AOSActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/PlayGameState.h"
#include "Wolf/Wolf.h"


AChar_Shinbi::AChar_Shinbi()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AChar_Shinbi::BeginPlay()
{
	Super::BeginPlay();

	if (SkillDataTable)
	{
		QSkill = SkillDataTable->FindRow<FSkillDataTable>(FName("QSkill"), TEXT(""));
	}
	
	if (QSkill)
	{
		SkillQMontage = QSkill->SkillMontage;
	}
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
		EnemyChannel = TeamCollision::RedTeam;
	}
	else
	{
		EnemyChannel = TeamCollision::BlueTeam;
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

/*void AChar_Shinbi::SkillQ()
{
	if (bEnableQSkill == true)
	{
		QSkillCooldownTime = QSkill->SkillCooldownTime;
		OnQSkillUsed.Broadcast(GetController()->GetName(), QSkillCooldownTime);

		Server_SkillQ();
	}
}*/

void AChar_Shinbi::Server_SkillQ_Implementation()
{
	if (bEnableQSkill == true)
	{
		QSkillCooldownTime = QSkill->SkillCooldownTime;
		GetWorld()->GetTimerManager().SetTimer(S_SkillQTimer, [this]()
		{
			bEnableQSkill = true;
		}, QSkillCooldownTime, false);
		
		SpawnWolfSkill();

		NM_SkillPlayMontage(SkillQMontage);
	}
}

void AChar_Shinbi::NM_SkillPlayMontage_Implementation(UAnimMontage* SkillMontage)
{
	if (!HasAuthority())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(SkillMontage);
	}
}

void AChar_Shinbi::SpawnWolfSkill()
{
	if (!HasAuthority()) return;
	
	FVector SpawnLocation = GetActorLocation() + GetActorRotation().Vector() * 150;
	SpawnLocation.Z = 0.0f;
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	
	AWolf* ShinbiWolf = GetWorld()->SpawnActor<AWolf>(WolfClass, SpawnLocation, SpawnRotation, SpawnParams);
}

void AChar_Shinbi::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bEnableQSkill);
}