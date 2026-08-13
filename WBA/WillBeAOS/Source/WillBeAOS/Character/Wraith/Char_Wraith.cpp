#include "Char_Wraith.h"

#include "Bomb_ESkill.h"
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
#include "Gimmick/Nexus.h"
#include "Gimmick/Projectile.h"
#include "Gimmick/Tower.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/GamePlayerState.h"
#include "PersistentGame/PlayGameState.h"
#include "Projectile/Projectile_Normal.h"
#include "Projectile/Projectile_QSkill.h"


AChar_Wraith::AChar_Wraith()
{	
	TrajectorySpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrajectorySpline"));
	TrajectorySpline->SetupAttachment(RootComponent);
	TrajectorySpline->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	bIsSetForward = false;
}

void AChar_Wraith::BeginPlay()
{
	Super::BeginPlay();
	
	GS = Cast<APlayGameState>(GetWorld()->GetGameState());

	UpdateMovementSpeedData(1.f);

	if (SkillDataTable)
	{
		QSkill = SkillDataTable->FindRow<FSkillDataTable>(FName("QSkill"), TEXT(""));
		ESkill = SkillDataTable->FindRow<FSkillDataTable>(FName("ESkill"), TEXT(""));

		SkillEMontage = ESkill->SkillMontage;
	}
	
	if (QSkill)
	{
		QSkillCooldownTime = QSkill->SkillCooldownTime;
		ESkillCooldownTime = ESkill->SkillCooldownTime;
	}

	if (!HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CleanupTimer, this, &ThisClass::CleanupFakeProjectiles, 10.f, true);
	}
}

void AChar_Wraith::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AChar_Wraith::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	if (!HasAuthority() && IsLocallyControlled())
	{
		TOptional<FHitResult> HitResult = CheckTargettingInCenter();

		if (HitResult.IsSet())
		{
			FVector HitPoint = HitResult->ImpactPoint;
			
			float ObjectDist = FVector::DistSquared(GetActorLocation(), HitPoint);
			if (ObjectDist <= TargettingTraceLength * TargettingTraceLength)
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

	// 총을 사용중인가
	if (IsCombat || bIsQSkillUsing || bIsESkillUsing)
	{
		if (!bUseGun) bUseGun = true;
	}
	else
	{
		if (bUseGun) bUseGun = false;	
	}

	// ESkill 장전중인가
	if (bIsESkillUsing && IsLocallyControlled())
	{
		UpdateTrajectory();
	}
}

void AChar_Wraith::StopMove(const FInputActionValue& Value)
{
	if (GetCharacterMovement())
	{
		if (bIsQSkillUsing || bIsESkillUsing)
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = true;
		}
		else
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
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
	FVector TraceEnd = TraceStart + WorldDirection * (TargettingTraceLength + CameraBoom->TargetArmLength);

	FHitResult HitActor;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GS)
	{
		for (TWeakObjectPtr<AActor> WeakActor : GS->CachedActors)
		{
			if (WeakActor.IsValid())
			{
				AActor* Ally = WeakActor.Get();
				
				if (!IsValid(Ally)) continue;
			
				// AAOSCharacter 중 아군 채널 제외
				AAOSCharacter* InGameChar = Cast<AAOSCharacter>(Ally);
				if (InGameChar)
				{
					if (InGameChar->TeamID == TeamID)
						QueryParams.AddIgnoredActor(Ally);
				}

				// AAOSActor 중 아군 채널 제외
				if (bIsQSkillUsing)
				{
					ATower* Tower = Cast<ATower>(Ally);
					if (Tower)
					{
						if (IsValid(Tower) && Tower->TeamID == TeamID)
							QueryParams.AddIgnoredActor(Tower);
					}
					ANexus* Nexus = Cast<ANexus>(Ally);
					if (Nexus)
					{
						if (IsValid(Nexus) && Nexus->TeamID == TeamID)
							QueryParams.AddIgnoredActor(Ally);
					}
				}
				else   // 줌아웃 시 타워 타겟팅 O
				{
					AAOSActor* InGameActor = Cast<AAOSActor>(Ally);
					if (InGameActor)
					{
						if (IsValid(InGameActor) && InGameActor->TeamID == TeamID)
							QueryParams.AddIgnoredActor(Ally);
					}
				}
			}
		}
	}

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);
	//ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

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

/*void AChar_Wraith::Attack()
{
	if (bIsQSkillUsing)
	{
		Execute_ActivateSkill(this, ESkillSlot::Q);
		SkillQAttack();
	}
	else if (bIsESkillUsing)
	{
		Execute_ActivateSkill(this, ESkillSlot::E);
		SkillEAttack();
	}
	else
	{
		Super::Attack();
	}
	
}

void AChar_Wraith::ClientAttack()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackCountTime) return;

	LastAttackTime = CurrentTime;
		
	// 라인 트레이스값 계산
	FVector WorldLocation, WorldDirection;
	FVector2D ScreenCenter;
	int32 ViewportX, ViewportY;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	PC->GetViewportSize(ViewportX, ViewportY);
	ScreenCenter = FVector2D(ViewportX, ViewportY) * 0.5f;

	PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

	FVector TraceStart = WorldLocation;
	FVector TraceEnd = TraceStart + WorldDirection * (NormalAttackDistance + CameraBoom->TargetArmLength);

	FVector MuzzleLocation = GetMesh()->GetSocketLocation("Muzzle_01");

	// 공격 실행 함수
	AttackFire(TraceEnd);
	Server_AttackFire(TraceStart, TraceEnd, MuzzleLocation);
}

void AChar_Wraith::Behavior()
{
	// Wraith는 따로 만들기 위해 비워둠
}*/

void AChar_Wraith::PlayNormalAttackAnim()
{
	if (CombatComp->GetAttackCount() < AttackMontages.Num())
	{
		float Progress = 0.f;
		if (Anim->Montage_IsPlaying(AttackMontages[CombatComp->GetAttackCount()]))
		{
			Progress = Anim->Montage_GetPosition(AttackMontages[CombatComp->GetAttackCount()]) / AttackMontages[CombatComp->GetAttackCount()]->GetPlayLength();
			if (Progress < 0.6f) return;
		}

		PlayAnimMontage(AttackMontages[CombatComp->GetAttackCount()]);
		
		CombatComp->AddAttackCount(1);
		if (CombatComp->GetAttackCount() >= AttackMontages.Num())
		{
			CombatComp->ResetCombo();
		}
	}
}

void AChar_Wraith::NM_HitEffect_Implementation(const FVector& HitLocation)
{
	if (HitParticle && !HasAuthority())
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitParticle,
			HitLocation,
			FRotator::ZeroRotator,
			FVector(0.7f),
			true);
	}
}

void AChar_Wraith::AttackFire(FVector TraceEnd)
{
	// 총알 방향 계산
	TOptional<FHitResult> HitResult = CheckTargettingInCenter();

	FVector StrikingPoint;
	if (HitResult.IsSet() && FVector::DistSquared(GetActorLocation(), HitResult->ImpactPoint) <= NormalAttackDistance * NormalAttackDistance)
	{
		StrikingPoint = HitResult->ImpactPoint;
		bIsStriking = true;
	}
	else
	{
		bIsStriking = false;
		StrikingPoint = TraceEnd;
	}

	FVector CharLocation = GetActorLocation();
	FVector ShootDirection = (StrikingPoint - CharLocation).GetSafeNormal();
	FVector FinalPoint;

	if (bIsStriking)
	{
		FinalPoint = StrikingPoint;
	}
	else
	{
		FinalPoint = CharLocation + (ShootDirection * NormalAttackDistance);
	}
	
	CaculatedBulletDirection(FinalPoint, bIsStriking, false);
	
	// 애니메이션 실행
	PlayNormalAttackAnim();
}

void AChar_Wraith::CaculatedBulletDirection(FVector Point, bool isStriking, bool isSkill)
{
	// 총알 발사
	FVector MuzzleLocation = GetMesh()->GetSocketLocation("Muzzle_01");
	FRotator FireRotation = (Point - MuzzleLocation).Rotation();

	if (!isSkill)
	{
		AProjectile_Normal* Proj = GetWorld()->SpawnActorDeferred<AProjectile_Normal>(Projectile_Normal, FTransform(FireRotation, MuzzleLocation));
		if (Proj)
		{
			Proj->SetOwner(this);
			Proj->BulletSpeed = BulletSpeed;
			if (isStriking)
			{
				//Proj->TraceLength = FVector::Dist(GetActorLocation(), Point);
			}
			else
			{
				//Proj->TraceLength = NormalAttackDistance;
			}
			Proj->FinishSpawning(FTransform(FireRotation, MuzzleLocation));
		}
	}
	else
	{
		AProjectile_QSkill* Proj = GetWorld()->SpawnActorDeferred<AProjectile_QSkill>(Projectile_QSkill, FTransform(FireRotation, MuzzleLocation));
		if (Proj)
		{
			Proj->SetOwner(this);
			Proj->BulletSpeed = BulletSpeed;
			if (isStriking)
			{
				Proj->TraceLength = FVector::Dist(GetActorLocation(), Point);
				Proj->MuzzleLocation = MuzzleLocation;
				Proj->EndLocation = Point;
			}
			else
			{
				Proj->TraceLength = QSkillDistance;
				Proj->MuzzleLocation = MuzzleLocation;
				Proj->EndLocation = MuzzleLocation + ((Point - MuzzleLocation).GetSafeNormal() * QSkillDistance);
			}
			Proj->FinishSpawning(FTransform(FireRotation, MuzzleLocation));
		}
	}
}

void AChar_Wraith::Server_AttackFire_Implementation(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation)
{		
	float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - LastAttackTime >= AttackCountTime - 0.05f)
	{
		LastAttackTime = CurrentTime;

		Server_EnterCombat();
		
		// 라인트레이스 계산
		ServerLineTraceHit(TraceStart, TraceEnd, MuzzleLocation);
	}
}

void AChar_Wraith::ServerLineTraceHit(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation)
{
	if (Projectile_Normal)
	{		
		FHitResult HitActor;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		if (GS)
		{
			for (auto WeakActor : GS->GameManagedActors)
			{
				if (WeakActor)
				{
					AActor* Ally = WeakActor;
			
					if (!Ally) continue;
		
					// AAOSCharacter 중 아군 채널 제외
					AAOSCharacter* InGameChar = Cast<AAOSCharacter>(Ally);
					if (InGameChar)
					{
						if (InGameChar->TeamID == TeamID)
						{
							QueryParams.AddIgnoredActor(Ally);
						}
					}

					// AAOSActor 중 아군 채널 제외
					AAOSActor* InGameActor = Cast<AAOSActor>(Ally);
					if (InGameActor)
					{
						if (IsValid(InGameActor) && InGameActor->TeamID == TeamID)
						{
							QueryParams.AddIgnoredActor(Ally);
						}
					}
				}
			}
		}
		
		FCollisionObjectQueryParams ObjectQuery;
		ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);
		//ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

		bool AttackSuccess = GetWorld()->LineTraceSingleByObjectType(
			HitActor,
			TraceStart,
			TraceEnd,
			ObjectQuery,
			QueryParams
		);

		FVector StrikingPoint;
		if (AttackSuccess && FVector::DistSquared(GetActorLocation(), HitActor.ImpactPoint) <= NormalAttackDistance * NormalAttackDistance)
		{
			bIsStriking = true;
			
			float Dist = FVector::Dist(MuzzleLocation, HitActor.ImpactPoint);
			float TravelTime = Dist / BulletSpeed;
			float ServerLoadTime = (TravelTime - 0.05f) <= 0 ? 0.01f : TravelTime - 0.05f ;

			StrikingPoint = HitActor.ImpactPoint;
			
			FTimerHandle HitTimer;

			GetWorld()->GetTimerManager().SetTimer(HitTimer, [this, HitActor]()
			{
				float Damage = 0;
				AController* Ctrl = Cast<AController>(GetController());
				if (Ctrl)
				{
					AGamePlayerState* PS = Ctrl->GetPlayerState<AGamePlayerState>();
					if (PS)
					{
						Damage = PS->CPower;
					}
				}
				if (Damage > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Damage: %f"), Damage);
					UGameplayStatics::ApplyPointDamage(
					HitActor.GetActor(),
					Damage,
					GetActorForwardVector(),
					HitActor,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass()
					);
				}

				// 멀티캐스트로 HitImpact 부분에 파티클 소환
				NM_HitEffect(HitActor.ImpactPoint);
			}, ServerLoadTime, false);
		}
		else
		{
			bIsStriking = false;
			StrikingPoint = TraceEnd;
		}

		FVector CharLocation = GetActorLocation();
		FVector ShootDirection = (StrikingPoint - CharLocation).GetSafeNormal();
		FVector FinalPoint;
		
		if (bIsStriking)
		{
			FinalPoint = StrikingPoint;
		}
		else
		{
			FinalPoint = CharLocation + (ShootDirection * NormalAttackDistance);
		}

		// 다른 클라이언트들에 총알 소환
		Multicast_AttackFire(FinalPoint, bIsStriking);
	}
}

void AChar_Wraith::Multicast_AttackFire_Implementation(FVector Point, bool isStriking)
{
	if (!IsLocallyControlled() && !HasAuthority())
	{
		CaculatedBulletDirection(Point, isStriking, false);

		PlayNormalAttackAnim();
	}
}

void AChar_Wraith::UseNewSkill(ESkillSlot NewSkill)
{
	if (CurrentUsingSkill == ESkillSlot::Q && NewSkill != ESkillSlot::Q)
	{
		ZoomOutScope();
	}
	else if (CurrentUsingSkill == ESkillSlot::E && NewSkill != ESkillSlot::E)
	{
		PutInTheBomb();
	}
}

void AChar_Wraith::ActivateSkill_Implementation(ESkillSlot SkillSlot)
{
	AGamePlayerState* PS = GetPlayerState<AGamePlayerState>();
	if (PS)
	{
		PS->Server_RequestUseSkill(SkillSlot);
	}
}

void AChar_Wraith::Handle_UseSkillButton(ESkillSlot Skillslot)
{
	Super::Handle_UseSkillButton(Skillslot);
	
	switch (Skillslot)
	{
	case ESkillSlot::Q:
		QSkill_Shot();
		break;
	case ESkillSlot::E:
		ESKill_Bomb();
		break;
	case ESkillSlot::R:

		break;
	}
}

void AChar_Wraith::QSkill_Shot()
{
	if (bIsDead) return;

	AGamePlayerState* PS = Cast<AGamePlayerState>(GetPlayerState());
	if (PS)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - PS->LastUseQSkillTime < QSkillCooldownTime) return;
	}
	
	UseNewSkill(ESkillSlot::Q);
	
	if (!bIsQSkillUsing)
	{
		ZoomInScope();
	}
	else
	{
		ZoomOutScope();
	}
}

void AChar_Wraith::ZoomInScope()
{
}

void AChar_Wraith::ZoomOutScope()
{
	bIsQSkillUsing = false;
	CurrentUsingSkill = ESkillSlot::None;
	SetZoomInBool(bIsQSkillUsing);
	

	TargettingTraceLength = NormalAttackDistance;
	GetWorld()->GetTimerManager().SetTimer(ZoomTimer, this, &ThisClass::UpdateZoom, 0.01f, true);
}

void AChar_Wraith::SetZoomInBool_Implementation(bool bZoomIn)
{
	bIsQSkillUsing = bZoomIn;

}

void AChar_Wraith::UpdateZoom()
{
	float TargetFOV = bIsQSkillUsing ? 45.f : 90.f;
	float CurrentFOV = FollowCamera->FieldOfView;
	float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, GetWorld()->DeltaTimeSeconds, 10.f);

	FollowCamera->SetFieldOfView(NewFOV);

	if (FMath::Abs(NewFOV - TargetFOV) <= 0.5f)
	{
		FollowCamera->SetFieldOfView(TargetFOV);
		GetWorld()->GetTimerManager().ClearTimer(ZoomTimer);
	}
}

void AChar_Wraith::SkillQAttack()
{
	if (bIsDead) return;
	
	Server_EnterCombat();

	ClientQSkill();
	ZoomOutScope();
}

void AChar_Wraith::ClientQSkill()
{
	AGamePlayerState* PS = Cast<AGamePlayerState>(GetPlayerState());
	if (PS)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - PS->LastUseQSkillTime < QSkillCooldownTime) return;

		PS->LastUseQSkillTime = CurrentTime;
	}
		
	// 라인 트레이스값 계산
	FVector WorldLocation, WorldDirection;
	FVector2D ScreenCenter;
	int32 ViewportX, ViewportY;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	PC->GetViewportSize(ViewportX, ViewportY);
	ScreenCenter = FVector2D(ViewportX, ViewportY) * 0.5f;

	PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

	FVector TraceStart = WorldLocation;
	FVector TraceEnd = TraceStart + WorldDirection * (QSkillDistance + CameraBoom->TargetArmLength);

	FVector MuzzleLocation = GetMesh()->GetSocketLocation("Muzzle_01");

	// 총알 방향 계산
	TOptional<FHitResult> HitResult = CheckTargettingInCenter();

	FVector StrikingPoint;
	if (HitResult.IsSet() && FVector::DistSquared(GetActorLocation(), HitResult->ImpactPoint) <= QSkillDistance * QSkillDistance)
	{
		StrikingPoint = HitResult->ImpactPoint;
		bIsStriking = true;
	}
	else
	{
		bIsStriking = false;
		StrikingPoint = TraceEnd;
	}

	FVector CharLocation = GetActorLocation();
	FVector ShootDirection = (StrikingPoint - CharLocation).GetSafeNormal();
	FVector FinalPoint;

	if (bIsStriking)
	{
		FinalPoint = StrikingPoint;
	}
	else
	{
		FinalPoint = CharLocation + (ShootDirection * QSkillDistance);
	}

	S_SkillQAttack(TraceStart, TraceEnd, MuzzleLocation);
	CaculatedBulletDirection(FinalPoint, bIsStriking, true);
	
	// 애니메이션 실행
	PlayQSKillAnim();
}

void AChar_Wraith::OnRep_QSkillUsing()
{
}

void AChar_Wraith::S_SkillQAttack_Implementation(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation)
{
	AGamePlayerState* PS = Cast<AGamePlayerState>(GetPlayerState());
	if (PS)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - PS->LastUseQSkillTime < QSkillCooldownTime) return;

		PS->LastUseQSkillTime = CurrentTime;
	}

	TargettingTraceLength = QSkillDistance;

	ServerLineTraceQSkill(TraceStart, TraceEnd, MuzzleLocation);

	TargettingTraceLength = NormalAttackDistance;
}

void AChar_Wraith::ServerLineTraceQSkill(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation)
{
	if (Projectile_QSkill)
	{	
		FHitResult HitActor;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		if (GS)
		{
			for (auto WeakActor : GS->GameManagedActors)
			{
				if (WeakActor)
				{
					AActor* Ally = WeakActor;
			
					if (!Ally) continue;
		
					// AAOSCharacter 중 아군 채널 제외
					AAOSCharacter* InGameChar = Cast<AAOSCharacter>(Ally);
					if (InGameChar)
					{
						if (InGameChar->TeamID == TeamID)
						{
							QueryParams.AddIgnoredActor(Ally);
						}
					}

					// AAOSActor 중 아군 채널 제외
					AAOSActor* InGameActor = Cast<AAOSActor>(Ally);
					if (InGameActor)
					{
						if (IsValid(InGameActor) && InGameActor->TeamID == TeamID)
						{
							QueryParams.AddIgnoredActor(Ally);
						}
					}
				}
			}
		}
		
		FCollisionObjectQueryParams ObjectQuery;
		ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);
		//ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

		bool AttackSuccess = GetWorld()->LineTraceSingleByObjectType(
			HitActor,
			TraceStart,
			TraceEnd,
			ObjectQuery,
			QueryParams
		);

		FVector StrikingPoint;
		if (AttackSuccess && FVector::DistSquared(GetActorLocation(), HitActor.ImpactPoint) <= QSkillDistance * QSkillDistance)
		{
			bIsStriking = true;
			
			float Dist = FVector::Dist(MuzzleLocation, HitActor.ImpactPoint);
			float TravelTime = Dist / BulletSpeed;
			float ServerLoadTime = (TravelTime - 0.05f) <= 0 ? 0.01f : TravelTime - 0.05f ;

			StrikingPoint = HitActor.ImpactPoint;
			
			FTimerHandle HitTimer;

			GetWorld()->GetTimerManager().SetTimer(HitTimer, [this, HitActor]()
			{
				float Damage = 0;
				AController* Ctrl = Cast<AController>(GetController());
				if (Ctrl)
				{
					AGamePlayerState* PS = Ctrl->GetPlayerState<AGamePlayerState>();
					if (PS)
					{
						Damage = PS->CPower * 2.f;
					}
				}

				if (Damage > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Damage: %f"), Damage);
					UGameplayStatics::ApplyPointDamage(
					HitActor.GetActor(),
					Damage,
					GetActorForwardVector(),
					HitActor,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass()
					);
				}

				// 멀티캐스트로 HitImpact 부분에 파티클 소환
				NM_HitEffect(HitActor.ImpactPoint);
			}, ServerLoadTime, false);
		}
		else
		{
			bIsStriking = false;
			StrikingPoint = TraceEnd;
		}

		FVector CharLocation = GetActorLocation();
		FVector ShootDirection = (StrikingPoint - CharLocation).GetSafeNormal();
		FVector FinalPoint;
		
		if (bIsStriking)
		{
			FinalPoint = StrikingPoint;
		}
		else
		{
			FinalPoint = CharLocation + (ShootDirection * QSkillDistance);
		}

		// 다른 클라이언트들에 총알 소환
		Multicast_QSkill(FinalPoint, bIsStriking);
	}
}

void AChar_Wraith::Multicast_QSkill_Implementation(FVector Point, bool isStriking)
{
	if (!IsLocallyControlled() && !HasAuthority())
	{
		CaculatedBulletDirection(Point, isStriking, true);

		PlayQSKillAnim();
	}
}

void AChar_Wraith::PlayQSKillAnim()
{
	PlayAnimMontage(QSkill->SkillMontage);
}

void AChar_Wraith::ESKill_Bomb()
{
	if (bIsDead) return;
	
	AGamePlayerState* PS = Cast<AGamePlayerState>(GetPlayerState());
	if (PS)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - PS->LastUseESkillTime < ESkillCooldownTime) return;
	}
	
	UseNewSkill(ESkillSlot::E);
	
	if (!bIsESkillUsing)
	{
		LoadToBomb();
	}
	else
	{
		PutInTheBomb();
	}
}

void AChar_Wraith::SetLoadToBombBool_Implementation(bool bLoad)
{
	bIsESkillUsing = bLoad;
}

void AChar_Wraith::LoadToBomb()
{
	bIsESkillUsing = true;
	SetLoadToBombBool(true);
	CurrentUsingSkill = ESkillSlot::E;
	TargettingTraceLength = ESkillTraceDistance;
}

void AChar_Wraith::PutInTheBomb()
{
	bIsESkillUsing = false;
	SetLoadToBombBool(false);
	ClearTrajectoryPath();
	TargettingTraceLength = NormalAttackDistance;
	CurrentUsingSkill = ESkillSlot::None;
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

void AChar_Wraith::SkillEAttack()
{
	if (bIsDead) return;

	Server_EnterCombat();

	ClientESkill();
	PutInTheBomb();
}

void AChar_Wraith::ClientESkill()
{
	AGamePlayerState* PS = Cast<AGamePlayerState>(GetPlayerState());
	if (PS)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - PS->LastUseESkillTime < ESkillCooldownTime) return;

		PS->LastUseESkillTime = CurrentTime;
	}

	FVector TraceStart = FollowCamera->GetComponentLocation();
	FVector TraceEnd = TraceStart + (FollowCamera->GetForwardVector() * (ESkillTraceDistance + CameraBoom->TargetArmLength));

	int64 UniqueProjectileID = GetUniqueProjectileID();

	if (IsLocallyControlled())
	{
		SpawnESkillBomb(UniqueProjectileID, TraceStart, TraceEnd);
		PlayThrowBombAnim();
	}
	
	Server_ESkillAttack(UniqueProjectileID, TraceStart, TraceEnd);
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
		ABomb_ESkill* Bomb = GetWorld()->SpawnActor<ABomb_ESkill>(Bomb_ESkillClass, SpawnTransform);
		
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

			AGamePlayerState* PS = GetPlayerState<AGamePlayerState>();
			if (PS)
			{
				float Damage = PS->CPower * 1.5f;
				Bomb->SetBombDamage(Damage);
			}
		}
	}
	else
	{
		FVector LookDir = (TargetLocation - StartLocation).GetSafeNormal();
		FTransform SpawnTransform(LookDir.Rotation(), StartLocation);
		ABomb_ESkill* Bomb = GetWorld()->SpawnActor<ABomb_ESkill>(Bomb_ESkillClass, SpawnTransform);
		
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

			AGamePlayerState* PS = GetPlayerState<AGamePlayerState>();
			if (PS)
			{
				float Damage = PS->CPower * 1.5f;
				Bomb->SetBombDamage(Damage);
			}
		}
	}
}

int64 AChar_Wraith::GetUniqueProjectileID()
{
	if (GetPlayerState())
	{
		int64 PlayerID = static_cast<int64>(GetPlayerState()->GetPlayerId());
		int64 CountID = static_cast<int64>(++ProjectileCounter);

		return (PlayerID << 32) | CountID;
	}
	return -1;
}

void AChar_Wraith::CleanupFakeProjectiles()
{
	if (FakeBombs.Num() == 0) return;

	for (auto It = FakeBombs.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		if (It.Value()->IsPendingKillPending())
		{
			It.RemoveCurrent();
		}
	}
}

void AChar_Wraith::PlayThrowBombAnim()
{
	PlayAnimMontage(SkillEMontage);
}

void AChar_Wraith::OnRep_ESkillUsing()
{
}

void AChar_Wraith::Multicast_ESkillAttack_Implementation(int64 UniqueID, FVector TraceStart, FVector TraceEnd)
{
	if (!IsLocallyControlled() && !HasAuthority())
	{
		SpawnESkillBomb(UniqueID, TraceStart, TraceEnd);
		PlayThrowBombAnim();
	}
}

void AChar_Wraith::Server_ESkillAttack_Implementation(int64 UniqueID, FVector TraceStart, FVector TraceEnd)
{
	SpawnESkillBomb(UniqueID, TraceStart, TraceEnd);
	Multicast_ESkillAttack(UniqueID, TraceStart, TraceEnd);
}

void AChar_Wraith::Multicast_ExplodeBomb_Implementation(int64 UniID)
{
	if (HasAuthority()) return;
	
	if (FakeBombs.Contains(UniID))
	{
		TWeakObjectPtr<ABomb_ESkill> TargetBomb = FakeBombs[UniID];
		
		if (TargetBomb.IsValid())
		{
			TargetBomb->SpawnParticle();
			TargetBomb->Destroy();
		}
		
		FakeBombs.Remove(UniID);
	}
}

void AChar_Wraith::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TargettingTraceLength);
}
