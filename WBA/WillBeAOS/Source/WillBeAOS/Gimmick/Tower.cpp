#include "Tower.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "NiagaraComponent.h"
#include "Projectile.h"
#include "../Character/CombatComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/ProgressBar.h"
#include "../Minions/HealthBar.h"
#include "Component/VisibleWidgetComponent.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/GamePlayerState.h"
#include "PersistentGame/PlayGameState.h"

ATower::ATower()
{
	bReplicates = true;           // 이 액터가 복제되도록 설정
	
	PrimaryActorTick.bCanEverTick = true;

	FakeRootCollision = CreateDefaultSubobject<USphereComponent>("FakeRootCollision");
	SetRootComponent(FakeRootCollision);
	
	HitCollision = CreateDefaultSubobject<UCapsuleComponent>("HitCollision");
	HitCollision->SetupAttachment(GetRootComponent());

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraParticleSystem"));
	NiagaraComponent->SetupAttachment(GetRootComponent());

	OverlapTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapTrigger"));
	OverlapTrigger->SetupAttachment(GetRootComponent());

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(GetRootComponent());

	AttackStartPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackStartPoint"));
	AttackStartPoint->SetupAttachment(GetRootComponent());

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	WidgetComponent->SetupAttachment(GetRootComponent());
	WidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 200.f));
	WidgetComponent->SetIsReplicated(false);
	WidgetComponent->SetVisibility(false);
	
	// 특정 요소의 오버랩 함수 바인드하기 ( OverlapTrigger의 )
	OverlapTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
	OverlapTrigger->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);

	WAbilitySystemComponent = CreateDefaultSubobject<UWAbilitySystemComponent>(TEXT("ASC"));
	WAttributeSet = CreateDefaultSubobject<UWAttributeSet>(TEXT("AttributeSet"));

	bAlwaysRelevant = true;

	SetGoldReward(GOLDAMOUNT);

	SightComp = CreateDefaultSubobject<UVisibleWidgetComponent>(TEXT("SightComponent"));

	StaticMesh->SetReceivesDecals(false);
}

UAbilitySystemComponent* ATower::GetAbilitySystemComponent() const
{
	return WAbilitySystemComponent;
}

void ATower::RegisterTagEvent()
{
	WAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UWAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ATower::OnHealthChange);
}

void ATower::OnHealthChange(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority()) return;

	float NewHealth = Data.NewValue;
	bool bFound;
	float MaxHealth = WAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) return;

	if (NewHealth <= 0)
	{
		AddGoldToEnemyPlayer();
		TowerDestroyMulticast();
	}
	else if (NewHealth <= MaxHealth / 2)
	{
		AddGoldToEnemyPlayer();
		S_SetDamaged();
	}
}

void ATower::BeginPlay()
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

		S_SetHPbarColor();
	}
	else
	{
		UHealthBar* HpInfoBar = Cast<UHealthBar>(WidgetComponent->GetWidget());
		if (HpInfoBar)
		{
			HpInfoBar->SetAndBoundToGameplayAttribute(WAbilitySystemComponent, UWAttributeSet::GetHealthAttribute(), UWAttributeSet::GetMaxHealthAttribute());
		}
	}

	RegisterTagEvent();
	SetTeamCollision();
}

void ATower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void ATower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() && OverlappingActors.IsValidIndex(0))
	{
		TargetOfActors = OverlappingActors[0].Get();

		// 타겟에 빔 조준
		if (TargetOfActors)
			BeamToTarget(TargetOfActors->GetActorLocation(), TargetOfActors);
	}
	
	if (HasAuthority() && OverlappingActors.IsValidIndex(0))
	{
		TargetOfActors = OverlappingActors[0].Get();
		
		// 공격 2초마다 한번씩 스폰
		Delta += DeltaTime;
		if(Delta >= 2)
		{
			Delta = 0;
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			FVector SpawnLocation = AttackStartPoint->GetComponentLocation();
			FRotator SpawnRotation = (TargetOfActors->GetActorLocation() - SpawnLocation).Rotation();
			
			AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>
			(SpawnActors, SpawnLocation, SpawnRotation, SpawnParams);

			if (Projectile)
			{
				bool bFound;
				float Value =  WAbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetAttackStatAttribute(), bFound);
				if (bFound) Projectile->ProjectileAttackStat = Value;
				Projectile->Target = TargetOfActors;
				Projectile->SetHomingTarget();
			}
		}
	}
}

void ATower::AddGoldToEnemyPlayer()
{
	if (!OverlappingActors.IsEmpty())
	{
		float CountOverlappingEnemyActors = 0;
		TArray<TWeakObjectPtr<AAOSCharacter>> OverlappingEnemyActors;
				
		for (auto& OverlappingActor : OverlappingActors)
		{
			if (Cast<AWCharacterBase>(OverlappingActor.Get()) && OverlappingActor->TeamID != TeamID)
			{
				CountOverlappingEnemyActors++;
				OverlappingEnemyActors.AddUnique(OverlappingActor);
			}
		}

		for (auto& OverlappingEnemyActor : OverlappingEnemyActors)
		{
			AGamePlayerState* PlayerState = OverlappingEnemyActor->GetInstigatorController()->GetPlayerState<AGamePlayerState>();
			if (PlayerState)
			{
				PlayerState->Server_AddGold(GetGoldReward() / CountOverlappingEnemyActors);
			}
		}
	}
}

void ATower::BeamToTarget(FVector TargetLocation, AAOSCharacter* Target)
{
	if (Cast<AWCharacterBase>(Target))
	{
		NiagaraComponent->SetVariableLinearColor("BeamColor", FLinearColor::Red);
	}
	else
	{
		NiagaraComponent->SetVariableLinearColor("BeamColor", FLinearColor::Blue);
	}
	
	FVector BeamStart = AttackStartPoint->GetComponentLocation(); // 빔 시작 위치
	FVector BeamEnd = TargetLocation;         // 빔 끝 위치
		
	NiagaraComponent->SetVectorParameter("MyBeamStart", BeamStart);
	NiagaraComponent->SetVectorParameter("MyBeamEnd", BeamEnd);
	NiagaraComponent->SetVisibility(true);
}

void ATower::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	
	AAOSCharacter* PlayChar = Cast<AAOSCharacter>(OtherActor);
	if (PlayChar && PlayChar->TeamID != TeamID)
	{
		// 적군 타겟 배열 등록
		OverlappingActors.AddUnique(PlayChar);
	}
	else if (PlayChar && PlayChar->TeamID == TeamID)
	{
		// 아군일 때 로직
		PlayChar->TowerWithCharacterInside = this;
	}
}

void ATower::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AAOSCharacter* DetectedChar = Cast<AAOSCharacter>(OtherActor);
	if (DetectedChar && OverlappingActors.Contains(DetectedChar))
		OverlappingActors.Remove(DetectedChar);
	else if (DetectedChar && DetectedChar->TeamID == TeamID && DetectedChar->TowerWithCharacterInside == this)
		DetectedChar->TowerWithCharacterInside = nullptr;
	
	// 타깃 배열이 비어있으면 스폰 시간 초기화 및 Niagara 비활성화
	if (OverlappingActors.IsEmpty())
	{
		Delta = 0;
		if (!HasAuthority())
		{
			NiagaraComponent->SetVisibility(false);
		}
	}
}

void ATower::TowerDestroyMulticast_Implementation()
{
	if (IsValid(StaticMesh))
	{
		FVector DestroyLocation = StaticMesh->GetComponentLocation();
		
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			DestroyParticle,
			DestroyLocation,
			FRotator::ZeroRotator,
			FVector(2.f),
			true);
	}

	Destroy();
}

void ATower::spawn()
{
	FActorSpawnParameters SpawnParams;
	GetWorld()->SpawnActor<AActor>(SpawnActors, AttackStartPoint->GetComponentTransform(), SpawnParams);
}

void ATower::SetTeamCollision()
{
	if (TeamID == E_TeamID::Blue)
	{
		HitCollision->SetCollisionObjectType(CollisionInfo::BlueTeam);
	}
	if (TeamID == E_TeamID::Red)
	{
		HitCollision->SetCollisionObjectType(CollisionInfo::RedTeam);
	}
}

void ATower::DamagedParticle_Implementation()
{
}

void ATower::S_SetHPbarColor_Implementation()
{
	static FLinearColor HealthBarColor;
	switch (TeamID)
	{
	case E_TeamID::Red:
		HealthBarColor = RedTeamColor;
		break;
	case E_TeamID::Blue:
		HealthBarColor = BlueTeamColor;
		break;
	case E_TeamID::Neutral:
		HealthBarColor = DefaultColor;
		break;
	}

	SetHPbarColor(HealthBarColor);
}

void ATower::SetHPbarColor_Implementation(FLinearColor HealthBarColor)
{
	UHealthBar* HealthBarWidget = Cast<UHealthBar>(WidgetComponent->GetWidget());
	if (!HealthBarWidget)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this, HealthBarColor]()
		{
			SetHPbarColor(HealthBarColor);
		});
		return;
	}
	
	if (HealthBarWidget->HealthBar)
	{
		HealthBarWidget->HealthBar->SetFillColorAndOpacity(HealthBarColor);

		HealthBarWidget->InvalidateLayoutAndVolatility();
	}
}

void ATower::S_SetDamaged_Implementation()
{
	NM_SetDamaged();
}

void ATower::NM_SetDamaged_Implementation()
{
	StaticMesh->SetStaticMesh(DamagedStaticMesh);
	DamagedParticle();
}

void ATower::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
	DOREPLIFETIME(ATower, TargetOfActors);
	DOREPLIFETIME(ATower, OverlappingActors);
}