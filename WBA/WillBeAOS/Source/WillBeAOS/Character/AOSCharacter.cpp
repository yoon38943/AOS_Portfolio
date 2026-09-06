#include "AOSCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"


AAOSCharacter::AAOSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	TeamTraceCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TeamTraceCollision"));
	TeamTraceCollision->SetupAttachment(GetRootComponent());

	TeamTraceCollision->SetReceivesDecals(false);
	GetCapsuleComponent()->SetReceivesDecals(false);
	GetMesh()->SetReceivesDecals(false);
}

void AAOSCharacter::RegisterAttacker(AGamePlayerState* NewAttacker)
{
	LastHitAttacker = NewAttacker;
}

void AAOSCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAOSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAOSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AAOSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, TeamID);
	DOREPLIFETIME(ThisClass, GoldReward);
	DOREPLIFETIME(ThisClass, bIsDead);
}