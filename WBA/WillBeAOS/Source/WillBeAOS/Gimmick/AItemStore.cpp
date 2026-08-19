#include "Gimmick/AItemStore.h"

#include "Character/WCharacterBase.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/GamePlayerState.h"

AItemStore::AItemStore()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(DefaultSceneRoot);

	SPCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SPCollision"));
	SPCollision->SetupAttachment(GetRootComponent());

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(GetRootComponent());
}

void AItemStore::BeginPlay()
{
	Super::BeginPlay();
}

void AItemStore::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!OtherActor) return;

	AWCharacterBase* PlayerChar = Cast<AWCharacterBase>(OtherActor);
	if(!PlayerChar || StoreTeam != PlayerChar->TeamID) return;
	
	GetWorld()->GetTimerManager().SetTimer(PlayerChar->HealingTimerHandle, [this, PlayerChar]()
	{
		if (HasAuthority())
		{
			AGamePlayerState* PS = Cast<AGamePlayerState>(PlayerChar->GetPlayerState());
			if (PS && PS->GetHP() < PS->GetMaxHP())
			{
				float HealAmount = PS->GetHP() + (PS->GetMaxHP() / 3);
				if (HealAmount <= PS->GetMaxHP())
				{
					PS->SetHP(HealAmount);
				}
				else
				{
					PS->SetHP(PS->GetMaxHP());
				}
			}
		}
	}, 1.f, true);

	if (!HasAuthority())
	{
		AGamePlayerController* PC = Cast<AGamePlayerController>(GetWorld()->GetFirstPlayerController());
		APawn* Player = PC->GetPawn();

		if (OtherActor == Player)
		{
			PC->IsOpenedStore = true;
		}
	}
}

void AItemStore::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (!OtherActor) return;

	AWCharacterBase* PlayerChar = Cast<AWCharacterBase>(OtherActor);
	if(!PlayerChar || StoreTeam != PlayerChar->TeamID) return;

	if (PlayerChar->HealingTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(PlayerChar->HealingTimerHandle);
	}

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("오버랩 종료"));
		AGamePlayerController* PC = Cast<AGamePlayerController>(GetWorld()->GetFirstPlayerController());
		APawn* Player = PC->GetPawn();

		if (OtherActor == Player)
		{
			PC->IsOpenedStore = false;
		}
	}
}