#pragma once

#include "CoreMinimal.h"
#include "Character/WCharacterBase.h"
#include "PersistentGame/PlayGameState.h"
#include "Character/Skill/SkillDataTable.h"
#include "Character/Struct_Enum/WalkSpeedStruct.h"
#include "Char_Wraith.generated.h"


class ABomb_QSkill;
class USplineMeshComponent;
class USplineComponent;
struct FPredictProjectilePathPointData;
class AProjectile_Normal;
class AProjectile_RMSkill;
enum class ShootingMode : uint8;

UCLASS()
class WILLBEAOS_API AChar_Wraith : public AWCharacterBase
{
	GENERATED_BODY()

public:
	AChar_Wraith();

protected:
	UPROPERTY()
	APlayGameState* GS;

	UPROPERTY(EditDefaultsOnly, Category = "HitParticle")
	UParticleSystem* HitParticle;

	virtual void RegisterTagEvent() override;

	UFUNCTION()
	void OnSinperTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnBombTagChanged(const FGameplayTag Tag, int32 NewCount);


protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	virtual void StopMove(const FInputActionValue& Value) override;

	void CheckTargeting();
	bool bDoOnceBindASC = true;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile_Normal> Projectile_Normal;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile_RMSkill> Projectile_QSkill;

public:
	UPROPERTY()
	AActor* LastTarget;
	// 타겟팅 관련
	TOptional<FHitResult> CheckTargettingInCenter();

	UPROPERTY(Replicated)
	float AttackDistance = 1200;

	float NormalAttackDistance = 1200.f;
	float SniperSkillDistance = 1500.f;

	// 공격
	bool CanAttack = true;

	float BulletSpeed = 12000.f;
	float ComboCount = 0;
	bool bIsStriking = false;
	

	float LastAttackTime = 0.f;
	float AttackCountTime = 0.73f;

public:
	// 스킬 관련
	ESkillSlot CurrentUsingSkill = ESkillSlot::None;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> Sniper_Layer;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> Bomb_Layer;
	
	void UpdateZoom();

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
	TMap<int64, TWeakObjectPtr<ABomb_QSkill>> FakeBombs;

	
	float ESkillCooldownTime;
	float ProjectileLaunchSpeed = 800.f;
	float ESkillTraceDistance = 900.f;
	float ProjectileCounter = 0.f;

	
	FSkillDataTable* ESkill;
	FTimerHandle TrajectoryTimerHandle;
	FTimerHandle CleanupTimer;
	

	void UpdateTrajectory();
	void DrawTrajectoryPath(const TArray<FPredictProjectilePathPointData>& PathData);
	void ClearTrajectoryPath();
	void SpawnESkillBomb(int64 UniqueID, FVector TraceStart, FVector TraceEnd);
};
