#include "Char_Wraith.h"

#include "Bomb_QSkill.h"
#include "Character/AOSActor.h"
#include "Character/WCharAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Character/CombatComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "Gimmick/Projectile.h"
#include "Gimmick/Tower.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/GamePlayerState.h"
#include "PersistentGame/PlayGameState.h"
#include "Projectile/Projectile_Normal.h"


AChar_Wraith::AChar_Wraith()
{	
	TrajectorySpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrajectorySpline"));
	TrajectorySpline->SetupAttachment(RootComponent);
	TrajectorySpline->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void AChar_Wraith::RegisterTagEvent()
{
	Super::RegisterTagEvent();
	
	AbilitySystemComponent->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("state.wraith.shootingmode.snipe")),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AChar_Wraith::OnSinperTagChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("state.wraith.shootingmode.bomb")),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AChar_Wraith::OnBombTagChanged);
}

void AChar_Wraith::OnSinperTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		if (RecallZoomTimer.IsValid()) GetWorld()->GetTimerManager().ClearTimer(RecallZoomTimer);
		GetMesh()->LinkAnimClassLayers(Sniper_Layer);
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
		MouseSensitivityMultiply = 0.3f;
		AttackDistance = SniperSkillDistance;
		bIsRMSkillUsing = true;
		GetWorld()->GetTimerManager().SetTimer(ZoomTimer, this, &ThisClass::UpdateZoom, 0.01f, true);
	}
	else
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("state.combat"))))
		{
			GetMesh()->LinkAnimClassLayers(Combat_Layer);
		}
		else
		{
			GetMesh()->LinkAnimClassLayers(NonCombat_Layer);
		}
		bool bFound;
		float SpeedValue = AbilitySystemComponent->GetGameplayAttributeValue(UWAttributeSet::GetSpeedStatAttribute(), bFound);
		if (bFound) GetCharacterMovement()->MaxWalkSpeed = SpeedValue;
		MouseSensitivityMultiply = 1.f;
		AttackDistance = NormalAttackDistance;
		bIsRMSkillUsing = false;
		GetWorld()->GetTimerManager().SetTimer(ZoomTimer, this, &ThisClass::UpdateZoom, 0.01f, true);
	}
}

void AChar_Wraith::OnBombTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		GetMesh()->LinkAnimClassLayers(Bomb_Layer);
	}
	else
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("state.combat"))))
		{
			GetMesh()->LinkAnimClassLayers(Combat_Layer);
		}
		else
		{
			GetMesh()->LinkAnimClassLayers(NonCombat_Layer);
		}
	}
}

void AChar_Wraith::BeginPlay()
{
	Super::BeginPlay();
	
	GS = Cast<APlayGameState>(GetWorld()->GetGameState());
}

void AChar_Wraith::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AChar_Wraith::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	CheckTargeting();

	// QSkill 장전중인가
	if (bIsQSkillUsing && IsLocallyControlled())
	{
		UpdateTrajectory();
	}
}

void AChar_Wraith::StopMove(const FInputActionValue& Value)
{
	if (GetCharacterMovement())
	{
		if (bIsQSkillUsing)
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = true;
		}
		else
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
		}
	}
}

void AChar_Wraith::CheckTargeting()
{
	if (!HasAuthority() && IsLocallyControlled())
	{
		TOptional<FHitResult> HitResult = CheckTargettingInCenter();

		if (HitResult.IsSet())
		{
			FVector HitPoint = HitResult->ImpactPoint;
			
			float ObjectDist = FVector::DistSquared(GetActorLocation(), HitPoint);
			if (ObjectDist <= AttackDistance * AttackDistance)
			{
				AActor* TargetEnemy = HitResult->GetActor();

				if (TargetEnemy && LastTarget != TargetEnemy)
				{
					if (bIsEnemyLockOn == false)
					{
						bIsEnemyLockOn = true;
					}

					if (AttackTarget.Num() > 0)
					{
						TArray<AActor*> DeleteActors;
						for (auto& Actor : AttackTarget)
						{
							DeleteActors.Add(Actor);
						}

						for (auto& Actor : DeleteActors)
						{
							AttackTarget.Remove(Actor);
						}
					}
					LastTarget = TargetEnemy;
					AttackTarget.AddUnique(LastTarget);
				}
			}
			else
			{
				if (bIsEnemyLockOn == true)
				{
					bIsEnemyLockOn = false;
				}
			
				if (AttackTarget.Num() > 0)
				{
					TArray<AActor*> DeleteActors;
					for (auto& Actor : AttackTarget)
					{
						DeleteActors.Add(Actor);
					}

					for (auto& Actor : DeleteActors)
					{
						AttackTarget.Remove(Actor);
					}
				}
				LastTarget = nullptr;
			}
		}
		else
		{
			if (bIsEnemyLockOn == true)
			{
				bIsEnemyLockOn = false;
			}
			
			if (AttackTarget.Num() > 0)
			{
				TArray<AActor*> DeleteActors;
				for (auto& Actor : AttackTarget)
				{
					DeleteActors.Add(Actor);
				}

				for (auto& Actor : DeleteActors)
				{
					AttackTarget.Remove(Actor);
				}
			}
			LastTarget = nullptr;
		}
	}
}

TOptional<FHitResult> AChar_Wraith::CheckTargettingInCenter()
{
	FVector WorldLocation, WorldDirection;
	FVector2D ScreenCenter;
	int32 ViewportX, ViewportY;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return TOptional<FHitResult>();

	PC->GetViewportSize(ViewportX, ViewportY);
	ScreenCenter = FVector2D(ViewportX, ViewportY) * 0.5f;

	PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

	FVector TraceStart = WorldLocation;
	FVector TraceEnd = TraceStart + WorldDirection * (AttackDistance + CameraBoom->TargetArmLength);

	FHitResult HitActor;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	ECollisionChannel EnemyChannel;
	if (GetTeamID() == E_TeamID::Blue)
	{
		EnemyChannel = CollisionInfo::RedTeam;
	}
	else
	{
		EnemyChannel = CollisionInfo::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(EnemyChannel);

	bool AttackSuccess = GetWorld()->LineTraceSingleByObjectType(
		HitActor,
		TraceStart,
		TraceEnd,
		ObjectQuery,
		QueryParams
	);

	if (AttackSuccess)
	{
		if (!Cast<AAOSCharacter>(HitActor.GetActor()) && !Cast<AAOSActor>(HitActor.GetActor()))
		{
			return TOptional<FHitResult>();
		}
		
		return HitActor;
	}
	else
	{
		return TOptional<FHitResult>();
	}
}

void AChar_Wraith::UpdateZoom()
{
	if (IsLocallyControlled())
	{
		float TargetFOV = bIsRMSkillUsing ? 45.f : 90.f;
		float CurrentFOV = FollowCamera->FieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, GetWorld()->DeltaTimeSeconds, 10.f);

		FollowCamera->SetFieldOfView(NewFOV);

		if (FMath::Abs(NewFOV - TargetFOV) <= 0.5f)
		{
			FollowCamera->SetFieldOfView(TargetFOV);
			GetWorld()->GetTimerManager().ClearTimer(ZoomTimer);
		}
	}
}

void AChar_Wraith::UpdateTrajectory()
{
	FVector TraceStart = FollowCamera->GetComponentLocation();
	FVector TraceEnd = TraceStart + (FollowCamera->GetForwardVector() * (ESkillTraceDistance + CameraBoom->TargetArmLength));

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FVector TargetLocation;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);
	
	if (bHit)
	{
		TargetLocation = HitResult.ImpactPoint;
	}
	else
	{
		TargetLocation = TraceEnd;
	}

	FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("Muzzle_03"));
	FVector OutLaunchVelocity;
	bool bHaveSolution = false;

	float Distance = FVector::Dist(StartLocation, TargetLocation);

	float MinDist = 500.f;
	float MaxDist = 1500.f;
	float MinSpeed = ProjectileLaunchSpeed;
	float MaxSpeed = ProjectileLaunchSpeed * 2.5f;

	float Alpha = FMath::Clamp((Distance - MinDist) / (MaxDist - MinDist), 0.f, 1.f);
	float CurrentSpeed = FMath::Lerp(MinSpeed, MaxSpeed, Alpha);
	
	bHaveSolution = UGameplayStatics::SuggestProjectileVelocity(
		this,
		OutLaunchVelocity,
		StartLocation,
		TargetLocation,
		CurrentSpeed,
		false, 0.f, 0.f,
		ESuggestProjVelocityTraceOption::DoNotTrace
	);

	FPredictProjectilePathParams PathParams;
	
	if (bHaveSolution)
	{
		PathParams.LaunchVelocity = OutLaunchVelocity;
	}
	else
	{
		FVector LookDir = (TargetLocation - StartLocation).GetSafeNormal();
		PathParams.LaunchVelocity = LookDir * ProjectileLaunchSpeed;
	}

	PathParams.StartLocation = StartLocation;
	PathParams.MaxSimTime = 3.f;
	PathParams.bTraceWithCollision = true;
	PathParams.ProjectileRadius = 5.f;
	PathParams.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	PathParams.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
	PathParams.ActorsToIgnore.Add(this);

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);

	DrawTrajectoryPath(PathResult.PathData);
}

void AChar_Wraith::DrawTrajectoryPath(const TArray<FPredictProjectilePathPointData>& PathData)
{
	if (!TrajectorySpline) return;
	
	TrajectorySpline->ClearSplinePoints(false);
	
	for (const auto& Point : PathData)
	{
		// 계산된 각 지점을 Spline 포인트로 추가
		TrajectorySpline->AddSplinePoint(Point.Location, ESplineCoordinateSpace::World, false);
	}
	TrajectorySpline->UpdateSpline();
	
	int NumSegments = PathData.Num() - 1;

	while (SplineMeshes.Num() < NumSegments)
	{
		USplineMeshComponent* NewMesh = NewObject<USplineMeshComponent>(this);
		NewMesh->SetStaticMesh(SplineSphereMesh);
		NewMesh->SetMaterial(0, SplineMaterial);
		NewMesh->SetMobility(EComponentMobility::Movable);
		NewMesh->SetupAttachment(TrajectorySpline);
		NewMesh->RegisterComponent();
		SplineMeshes.Add(NewMesh);
	}

	for (int i = 0; i < SplineMeshes.Num(); ++i)
	{
		if (i < NumSegments)
		{
			SplineMeshes[i]->SetVisibility(true);

			FVector StartPos, StartTangent, EndPos, EndTangent;
			TrajectorySpline->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::World);
			TrajectorySpline->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::World);

			FVector2D ThinScale(0.02f, 0.02f); 
			SplineMeshes[i]->SetStartScale(ThinScale);
			SplineMeshes[i]->SetEndScale(ThinScale);

			FTransform SplineTransform = TrajectorySpline->GetComponentTransform();
			FVector LocalStart = SplineTransform.InverseTransformPosition(StartPos);
			FVector LocalEnd = SplineTransform.InverseTransformPosition(EndPos);
			FVector LocalStartTangent = SplineTransform.InverseTransformVector(StartTangent);
			FVector LocalEndTangent = SplineTransform.InverseTransformVector(EndTangent);
			
			SplineMeshes[i]->SetStartAndEnd(LocalStart, LocalStartTangent, LocalEnd, LocalEndTangent, true);
		}
		else
		{
			SplineMeshes[i]->SetVisibility(false);
		}
	}
}

void AChar_Wraith::ClearTrajectoryPath()
{
	if (TrajectorySpline) TrajectorySpline->ClearSplinePoints();
    
	for (auto SplineMesh : SplineMeshes)
	{
		if (SplineMesh) SplineMesh->SetVisibility(false);
	}
}

void AChar_Wraith::SpawnESkillBomb(int64 UniqueID, FVector TraceStart, FVector TraceEnd)
{
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FVector TargetLocation;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);
	
	if (bHit)
	{
		TargetLocation = HitResult.ImpactPoint;
	}
	else
	{
		TargetLocation = TraceEnd;
	}

	FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("Muzzle_03"));
	FVector OutLaunchVelocity;
	bool bHaveSolution = false;

	float Distance = FVector::Dist(StartLocation, TargetLocation);

	float MinDist = 500.f;
	float MaxDist = 1500.f;
	float MinSpeed = ProjectileLaunchSpeed;
	float MaxSpeed = ProjectileLaunchSpeed * 2.5f;

	float Alpha = FMath::Clamp((Distance - MinDist) / (MaxDist - MinDist), 0.f, 1.f);
	float CurrentSpeed = FMath::Lerp(MinSpeed, MaxSpeed, Alpha);
	
	bHaveSolution = UGameplayStatics::SuggestProjectileVelocity(
		this,
		OutLaunchVelocity,
		StartLocation,
		TargetLocation,
		CurrentSpeed,
		false, 0.f, 0.f,
		ESuggestProjVelocityTraceOption::DoNotTrace
	);
	
	if (bHaveSolution)
	{
		FTransform SpawnTransform(OutLaunchVelocity.Rotation(), StartLocation);
		ABomb_QSkill* Bomb = GetWorld()->SpawnActor<ABomb_QSkill>(Bomb_ESkillClass, SpawnTransform);
		
		if(Bomb)
		{
			if (!HasAuthority())
			{
				FakeBombs.Add(UniqueID, Bomb);
				Bomb->UniqueID = UniqueID;
			}
			
			Bomb->SetOwner(this);
			Bomb->TeamID = TeamID;
			Bomb->ProjectileMovement->Velocity = OutLaunchVelocity;
			
			float LaunchSpeed = OutLaunchVelocity.Size();
			Bomb->ProjectileMovement->MaxSpeed = LaunchSpeed * 1.2f;

			Bomb->CollisionComp->IgnoreActorWhenMoving(this, true);
			this->MoveIgnoreActorAdd(Bomb);
		}
	}
	else
	{
		FVector LookDir = (TargetLocation - StartLocation).GetSafeNormal();
		FTransform SpawnTransform(LookDir.Rotation(), StartLocation);
		ABomb_QSkill* Bomb = GetWorld()->SpawnActor<ABomb_QSkill>(Bomb_ESkillClass, SpawnTransform);
		
		if(Bomb)
		{
			if (!HasAuthority())
			{
				FakeBombs.Add(UniqueID, Bomb);
				Bomb->UniqueID = UniqueID;
			}

			Bomb->SetOwner(this);
			Bomb->TeamID = TeamID;
			
			// LookDir은 단위벡터이므로 원하는 발사 속도로 곱해서 실제 속도를 설정
			float LaunchSpeed = FMath::Max(ProjectileLaunchSpeed, 100.f); // 최소 속도 보장
			FVector LaunchVelocity = LookDir * LaunchSpeed;

			Bomb->ProjectileMovement->Velocity = LaunchVelocity;
			Bomb->ProjectileMovement->MaxSpeed = LaunchSpeed * 1.2f;

			Bomb->CollisionComp->IgnoreActorWhenMoving(this, true);
			this->MoveIgnoreActorAdd(Bomb);
		}
	}
}

void AChar_Wraith::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, AttackDistance);
}
