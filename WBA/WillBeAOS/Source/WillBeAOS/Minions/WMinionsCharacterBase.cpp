#include "WMinionsCharacterBase.h"

#include "AbilitySystemGlobals.h"
#include "../Character/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "WMinionsAIController.h"
#include "BrainComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/ProgressBar.h"
#include "HealthBar.h"
#include "Character/WCharacterBase.h"
#include "Component/VisibleWidgetComponent.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/PlayGameMode.h"
#include "PersistentGame/PlayGameState.h"


AWMinionsCharacterBase::AWMinionsCharacterBase()
{
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetCombatEnable(true);
	CombatComponent->SetCollisionMesh(GetMesh());

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	WidgetComponent->SetupAttachment(GetMesh());
	WidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 200.f));
	WidgetComponent->SetVisibility(false, true);

	WAbilitySystemComponent = CreateDefaultSubobject<UWAbilitySystemComponent>(TEXT("ASC"));
	WAttributeSet = CreateDefaultSubobject<UWAttributeSet>(TEXT("AttributeSet"));

	bAlwaysRelevant = true;
	
	SetGoldReward(MINIONKILLGOLD);

	SightComp = CreateDefaultSubobject<UVisibleWidgetComponent>(TEXT("SightComponent"));
}

void AWMinionsCharacterBase::SetTeamCollision()
{
	if (TeamID == E_TeamID::Blue)
	{
		TeamTraceCollision->SetCollisionObjectType(TeamCollision::BlueTeam);
	}
	if (TeamID == E_TeamID::Red)
	{
		TeamTraceCollision->SetCollisionObjectType(TeamCollision::RedTeam);
	}
}

UAbilitySystemComponent* AWMinionsCharacterBase::GetAbilitySystemComponent() const
{
	return WAbilitySystemComponent;
}

void AWMinionsCharacterBase::RegisterTagEvent()
{
	WAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UWAttributeSet::GetHealthAttribute()
		).AddUObject(this, &AWMinionsCharacterBase::OnHealthAttributeChanged);
}

void AWMinionsCharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority()) return;

	if (bIsDead) return;
	
	float NewHealth = Data.NewValue;
	if (NewHealth <= 0.f)
	{
		bIsDead = true;
		Dead();
	}
}

void AWMinionsCharacterBase::HandleGameEnd()
{
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->BrainComponent->StopLogic("Game Ended");

		PrimaryActorTick.bCanEverTick = false;
	}
}

void AWMinionsCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	WAbilitySystemComponent->InitAbilityActorInfo(this, this);

	APlayGameMode* GM = Cast<APlayGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM)
	{
		GM->OnGameEnd.AddUObject(this, &ThisClass::HandleGameEnd);
	}

	RegisterTagEvent();

	SetTeamCollision();
	FindPlayerPC();

	if (HasAuthority())
	{		
		APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->GameManagedActors.AddUnique(this);
		}

		FGameplayAbilitySpec Spec(Minion_BasicAttack_Class, 1, INDEX_NONE, this);
		WAbilitySystemComponent->GiveAbility(Spec);

		WAbilitySystemComponent->ApplyInitialStat(StatTable, InitStatEffect, CharacterName);
		WAbilitySystemComponent->ApplyInitialEffects(InitialEffects);
		
		GetWorld()->GetTimerManager().SetTimer(
			CheckDistanceTimerHandle,
			this,
			&ThisClass::CheckDistanceToTarget,
			0.2f,
			true
		);
	}

	if (!HasAuthority())
	{
		UHealthBar* HpInfoBar = Cast<UHealthBar>(WidgetComponent->GetWidget());
		if (HpInfoBar)
		{
			HpInfoBar->SetAndBoundToGameplayAttribute(WAbilitySystemComponent, UWAttributeSet::GetHealthAttribute(), UWAttributeSet::GetMaxHealthAttribute());
		}
		StartSetHPbarColor();
	}
}

void AWMinionsCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AWMinionsCharacterBase::FindPlayerPC()
{
	if (HasAuthority()) return;
	
	PlayerController = Cast<AGamePlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (!PlayerController && MinionPCTimerManager.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is null"));
		GetWorldTimerManager().SetTimer(MinionPCTimerManager, this, &ThisClass::FindPlayerPC, 0.2f, false);
	}
	else
	{
		if (MinionPCTimerManager.IsValid())
			GetWorldTimerManager().ClearTimer(MinionPCTimerManager);
	}
}

void AWMinionsCharacterBase::FindPlayerPawn()
{
	if (HasAuthority()) return;
	
	if (PlayerController)
	{
		PlayerChar = Cast<AWCharacterBase>(PlayerController->GetPawn());
	}
}

void AWMinionsCharacterBase::CheckDistanceToTarget()
{
	if (bIsDead) return;
		
	FVector MyLocation = GetActorLocation();
	float VisibleDistanceSqr = FMath::Square(VisibleWidgetDistance);

	APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
	if (GS && GS->GameManagedActors.Num() > 0)
	{
		for (AActor* Actor : GS->GameManagedActors)
		{
			if (!IsValid(Actor) || Actor == this) continue;

			float DistSqr = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
			bool bShouldShow = DistSqr <= VisibleDistanceSqr;

			if (bShouldShow && !CurrentObserveObjects.Contains(Actor))
			{
				CurrentObserveObjects.Add(Actor);
				CanAttackToTarget(Actor);
			}
			else if (!bShouldShow && CurrentObserveObjects.Contains(Actor))
			{
				CurrentObserveObjects.Remove(Actor);
				DetachToTarget(Actor);
			}
		}
	}
}

void AWMinionsCharacterBase::DetachToTarget_Implementation(AActor* WObject)
{
	// 블루프린트 내 구현
}

void AWMinionsCharacterBase::CanAttackToTarget_Implementation(AActor* WObject)
{
	// 블루프린트 내 구현
}

void AWMinionsCharacterBase::StartSetHPbarColor()
{
	if (bIsDead) return;
	
	static FLinearColor HealthBarColor;
	switch (TeamID)
	{
	case E_TeamID::Red:
		HealthBarColor = FLinearColor::Red;
		break;
	case E_TeamID::Blue:
		HealthBarColor = FLinearColor::Blue;
		break;
	case E_TeamID::Neutral:
		HealthBarColor = FLinearColor::Yellow;
		break;
	}

	SetHPbarColor(HealthBarColor);
}

void AWMinionsCharacterBase::SetHPbarColor(FLinearColor HealthBarColor)
{
	if (!WidgetComponent || !WidgetComponent->GetWidget()) {
		GetWorld()->GetTimerManager().SetTimer(HPbarColorTimerHandle, [this, HealthBarColor]()
		{
			SetHPbarColor(HealthBarColor);
		}, 0.1f, false);
		return;
	}

	UHealthBar* Widget = Cast<UHealthBar>(WidgetComponent->GetWidget());
	if (!Widget || !Widget->HealthBar)
	{
		return;
	}
	
	if (Widget->HealthBar)
	{
		Widget->HealthBar->SetFillColorAndOpacity(HealthBarColor);

		Widget->InvalidateLayoutAndVolatility();
	}
}

void AWMinionsCharacterBase::Dead()
{
	if (!HasAuthority()) return;

	// 골드
	APlayGameMode* GameMode = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->OnObjectKilled(this, LastHitAttacker.Get());
	}

	// CheckDistance 셋타이머 끄기
	if (CheckDistanceTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckDistanceTimerHandle);
	}

	APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
	if (GS)
	{
		GS->GameManagedActors.Remove(this);
	}
	
	// AI가 죽으면 BT 연결 끊기
	AWMinionsAIController* MinionController = Cast<AWMinionsAIController>(GetController());
	if (MinionController)
	{
		MinionController->GetBrainComponent()->StopLogic(TEXT("None"));
	}

	bIsDead = true;

	FTimerHandle MinionDeadTimer;
	GetWorld()->GetTimerManager().SetTimer(MinionDeadTimer, [this]()
	{
		Destroy();
	}, 2.5f, false);
	
	NM_BeingDead();
}

void AWMinionsCharacterBase::NM_BeingDead_Implementation()
{
	//콜리전 없애기
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetTeamIDCollision()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	// 죽는 애니메이션 실행
	//PlayAnimMontage(DeadAnimMontage);

	if (!HasAuthority())
	{
		// HP Widget 없애기
		WidgetComponent->SetHiddenInGame(true);
	}
}

void AWMinionsCharacterBase::NM_Minion_Attack_Implementation()
{
	if (WAbilitySystemComponent)
	{
		WAbilitySystemComponent->TryActivateAbilityByClass(Minion_BasicAttack_Class);
	}
}

void AWMinionsCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,TrackNum);
}
