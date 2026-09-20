#include "WMinionsCharacterBase.h"
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
#include "Engine/OverlapResult.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/PlayGameMode.h"
#include "PersistentGame/PlayGameState.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"


AWMinionsCharacterBase::AWMinionsCharacterBase()
{
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	WidgetComponent->SetupAttachment(GetMesh());
	WidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 200.f));
	WidgetComponent->SetVisibility(false, true);

	WAbilitySystemComponent = CreateDefaultSubobject<UWAbilitySystemComponent>(TEXT("ASC"));
	WAttributeSet = CreateDefaultSubobject<UWAttributeSet>(TEXT("AttributeSet"));
	
	TeamTraceCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TeamTraceCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	TeamTraceCollision->SetCollisionResponseToChannel(CollisionInfo::Perception, ECR_Overlap);

	bAlwaysRelevant = true;
	
	SetGoldReward(MINIONKILLGOLD);

	SightComp = CreateDefaultSubobject<UVisibleWidgetComponent>(TEXT("SightComponent"));
}

void AWMinionsCharacterBase::SetTeamCollision()
{
	TeamTraceCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	if (TeamID == E_TeamID::Blue)
	{
		TeamTraceCollision->SetCollisionObjectType(CollisionInfo::BlueTeam);
		EnemyCollision = CollisionInfo::RedTeam;
	}
	else
	{
		TeamTraceCollision->SetCollisionObjectType(CollisionInfo::RedTeam);
		EnemyCollision = CollisionInfo::BlueTeam;
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
			&ThisClass::CheckOverlappedTarget,
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

void AWMinionsCharacterBase::CheckOverlappedTarget()
{
	if (bIsDead) return;

	if (AAOSCharacter* CharTarget = Cast<AAOSCharacter>(TargetActor.Get()))
	{
		bool bTargetValid = !CharTarget->bIsDead &&
			FVector::DistSquared(GetActorLocation(), CharTarget->GetActorLocation()) <= FMath::Square(DetectionRadius);

		if (bTargetValid) return;		
	}
	else if (AAOSActor* ActorTarget = Cast<AAOSActor>(TargetActor.Get()))
	{
		return;
	}

	TargetActor.Reset();
	bIsTargetDetected = false;

	FindNewTarget();
}

void AWMinionsCharacterBase::FindNewTarget()
{
	FCollisionObjectQueryParams DetectionObjectParams = FCollisionObjectQueryParams();
	DetectionObjectParams.AddObjectTypesToQuery(EnemyCollision);

	TArray<FOverlapResult> OverlapResults;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MinionDetectionOverlap), false);
	QueryParams.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		DetectionObjectParams,
		FCollisionShape::MakeSphere(DetectionRadius),
		QueryParams);

	TWeakObjectPtr<AActor> NearestActor = nullptr;
	float LowDistance = MAX_FLT;
	for (const auto& Result : OverlapResults)
	{
		if (!IsValid(Result.GetActor())) continue;
		
		AActor* OtherActor = Result.GetActor();
		AAOSCharacter* OtherCharacter = Cast<AAOSCharacter>(OtherActor);
		if (OtherCharacter && OtherCharacter->bIsDead == true) continue;

		float Distance = FVector::DistSquared(GetActorLocation(), OtherActor->GetActorLocation());
		if (Distance < LowDistance)
		{
			NearestActor = OtherActor;
			LowDistance = Distance;
		}
	}

	TargetActor = NearestActor;
	if (TargetActor.IsValid()) bIsTargetDetected = true;
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
	UHealthBar* Widget = Cast<UHealthBar>(WidgetComponent->GetWidget());
	if (!Widget || !Widget->HealthBar) return;
	
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
