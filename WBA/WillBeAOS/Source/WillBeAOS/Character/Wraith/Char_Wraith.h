#pragma once

#include "CoreMinimal.h"
#include "Character/WCharacterBase.h"
#include "PersistentGame/PlayGameState.h"
#include "Character/Skill/SkillDataTable.h"
#include "Character/Struct_Enum/WalkSpeedStruct.h"
#include "Enum/ShootingMode.h"
#include "Char_Wraith.generated.h"


class ABomb_ESkill;
class USplineMeshComponent;
class USplineComponent;
struct FPredictProjectilePathPointData;
class AProjectile_Normal;
class AProjectile_QSkill;
enum class ShootingMode : uint8;

UCLASS()
class WILLBEAOS_API AChar_Wraith : public AWCharacterBase
{
	GENERATED_BODY()

public:
	AChar_Wraith();

protected:
	APlayGameState* GS;

	UPROPERTY(EditDefaultsOnly, Category = "HitParticle")
	UParticleSystem* HitParticle;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	virtual void StopMove(const FInputActionValue& Value) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile_Normal> Projectile_Normal;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile_QSkill> Projectile_QSkill;

public:	
	AActor* LastTarget;
	// 타겟팅 관련
	TOptional<FHitResult> CheckTargettingInCenter();

	UPROPERTY(Replicated)
	float TargettingTraceLength = 1200.f;

	float NormalAttackDistance = 1200.f;
	float QSkillDistance = 1500.f;

	// 공격
	bool CanAttack = true;

	/*virtual void Attack() override;

	virtual void ClientAttack() override;
	virtual void Behavior() override;*/
	void PlayNormalAttackAnim();

	float BulletSpeed = 12000.f;
	float ComboCount = 0;
	bool bIsStriking = false;
	
	void AttackFire(FVector TraceEnd);

	void CaculatedBulletDirection(FVector Point, bool isStriking, bool isSkill);

	float LastAttackTime = 0.f;
	float AttackCountTime = 0.73f;
	UFUNCTION(Server, Reliable)
	void Server_AttackFire(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation);
	void ServerLineTraceHit(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation);

	UFUNCTION(NetMulticast,Reliable)
	void Multicast_AttackFire(FVector Point, bool isStriking);

	UFUNCTION(NetMulticast, Reliable)
	void NM_HitEffect(const FVector& HitLocation);

public:
	// 스킬 관련
	ESkillSlot CurrentUsingSkill = ESkillSlot::None;

	void UseNewSkill(ESkillSlot NewSkill);
	
	virtual void ActivateSkill_Implementation(ESkillSlot SkillSlot) override;
	
	virtual void Handle_UseSkillButton(ESkillSlot Skillslot) override;	// 스킬 input switch 함수
	
	// Q스킬
	UPROPERTY(EditAnywhere)
	UAnimMontage* ZoomInMontage;
	void QSkill_Shot();

	FMovementSpeedStruct MovementSpeedData;

	UPROPERTY(BlueprintReadOnly)
	bool bUseGun = false;
	FSkillDataTable* QSkill;

	float QSkillCooldownTime;
	FTimerHandle S_SkillQTimer;

	FTimerHandle ZoomTimer;

	void ZoomInScope();
	void ZoomOutScope();
	UFUNCTION(Server, Reliable)
	void SetZoomInBool(bool bZoomIn);
	void UpdateZoom();
	void SkillQAttack();
	void ClientQSkill();
	virtual void OnRep_QSkillUsing() override;
	
	UFUNCTION(Server, Reliable)
	void S_SkillQAttack(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation);
	void ServerLineTraceQSkill(FVector TraceStart, FVector TraceEnd, FVector MuzzleLocation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_QSkill(FVector Point, bool isStriking);

	void PlayQSKillAnim();

	// E스킬
	UPROPERTY(EditAnywhere)
	UAnimMontage* ESkillReadyMontage;
	UPROPERTY()
	UAnimMontage* SkillEMontage;
	UPROPERTY(VisibleAnywhere, Category = Trajectory)
	USplineComponent* TrajectorySpline;
	UPROPERTY(EditAnywhere, Category = Trajectory)
	UStaticMesh* SplineSphereMesh;
	UPROPERTY(EditAnywhere, Category = Trajectory)
	UMaterialInterface* SplineMaterial;
	UPROPERTY()
	TArray<USplineMeshComponent*> SplineMeshes;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Bomb_ESkillClass;
	TMap<int64, TWeakObjectPtr<ABomb_ESkill>> FakeBombs;

	
	float ESkillCooldownTime;
	float ProjectileLaunchSpeed = 800.f;
	float ESkillTraceDistance = 900.f;
	float ProjectileCounter = 0.f;

	
	FSkillDataTable* ESkill;
	FTimerHandle TrajectoryTimerHandle;
	FTimerHandle CleanupTimer;
	

	void ESKill_Bomb();
	void LoadToBomb();
	void PutInTheBomb();
	void UpdateTrajectory();
	void DrawTrajectoryPath(const TArray<FPredictProjectilePathPointData>& PathData);
	void ClearTrajectoryPath();
	void SkillEAttack();
	void ClientESkill();
	void SpawnESkillBomb(int64 UniqueID, FVector TraceStart, FVector TraceEnd);
	int64 GetUniqueProjectileID();
	void CleanupFakeProjectiles();
	void PlayThrowBombAnim();
	virtual void OnRep_ESkillUsing() override;

	
	UFUNCTION(Server, Reliable)
	void SetLoadToBombBool(bool bLoad);
	UFUNCTION(Server, Reliable)
	void Server_ESkillAttack(int64 UniqueID, FVector TraceStart, FVector TraceEnd);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ESkillAttack(int64 UniqueID, FVector TraceStart, FVector TraceEnd);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ExplodeBomb(int64 UniID);
};
