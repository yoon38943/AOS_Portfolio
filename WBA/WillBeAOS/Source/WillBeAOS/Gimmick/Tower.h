#pragma once

#include "CoreMinimal.h"
#include "Character/AOSActor.h"
#include "Character/WCharacterBase.h"
#include "GameFramework/Actor.h"
#include "Tower.generated.h"

class USceneComponent;
class UCapsuleComponent;
class USphereComponent;
class UStaticMeshComponent;

#define GOLDAMOUNT 150
UCLASS()
class WILLBEAOS_API ATower : public AAOSActor, public IVisibleSightInterface, public IAbilitySystemInterface
{

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Color")
	FLinearColor BlueTeamColor;
	UPROPERTY(EditAnywhere, Category = "Color")
	FLinearColor RedTeamColor;
	UPROPERTY(EditAnywhere, Category = "Color")
	FLinearColor DefaultColor;
	
public:
	ATower();
	
	UPROPERTY(BlueprintReadWrite, Category = "GameState")
	class APlayGameState* AWGS;

	/*********************************************************/
	// GAS 시스템
	/*********************************************************/
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability")
	UWAbilitySystemComponent* WAbilitySystemComponent;
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability")
	UWAttributeSet* WAttributeSet;
	
protected://체력관련
	
	UFUNCTION(NetMulticast, Reliable)
	void SetHpPercentage(float Health, float MaxHealth);
	UFUNCTION(Server, Reliable)
	void S_SetHpPercentage(float Health, float MaxHealth);

	UFUNCTION(Server, Reliable)
	void S_InitHPPercentage();
	UFUNCTION(NetMulticast, Reliable)
	void C_InitHPPercentage(float Health, float MaxHealth);
	void InitHPPercentage(float Health, float MaxHealth);
	
public:
	UFUNCTION(Server, Reliable)
	void S_SetHPbarColor();
	UFUNCTION(NetMulticast, Reliable)
	void SetHPbarColor(FLinearColor HealthBarColor);

	UFUNCTION(Server, Reliable)
	void Server_UpdateHPBar();
	UFUNCTION(Client, Reliable)
	void Client_UpdateWidget(float HPPercent);

	UFUNCTION(Server, Reliable)
	void S_SetDamaged();
	UFUNCTION(NetMulticast, Reliable)
	void NM_SetDamaged();

	UFUNCTION(NetMulticast, Reliable)
	void TowerDestroyInClient();
	
public://스폰
	float Delta;
	
	void spawn();

	UStaticMeshComponent* GetMesh() { return StaticMesh; }

	void SetTeamCollision();
	
public:
	UPROPERTY(EditDefaultsOnly)
	USphereComponent* FakeRootCollision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UNiagaraComponent* NiagaraComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* OverlapTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCapsuleComponent* HitCollision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* AttackStartPoint;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UWidgetComponent* WidgetComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UCombatComponent* CombatComp;
	UPROPERTY(BlueprintReadWrite)
	UStaticMesh* DamagedStaticMesh;
	UFUNCTION(BlueprintNativeEvent)
	void DamagedParticle();
	UPROPERTY(EditAnywhere)
	UParticleSystem* DestroyParticle;
	
	bool IsParticleSpawned = false;

public://타격 관련
	UPROPERTY(BlueprintReadWrite, Category = SpawnActor)
	TSubclassOf<AActor> SpawnActors;
	UPROPERTY(BlueprintReadOnly, Category = SpawnActor, Replicated)
	AAOSCharacter* TargetOfActors;
	FVector HitLocation;
	FVector HitNormal;
	FName BoneName;
	FHitResult OutHit;
	// 오버랩된 액터들의 배열 ( 공격 대상들 )
	UPROPERTY(Replicated)
	TArray<TWeakObjectPtr<AAOSCharacter>> OverlappingActors = {};
	ETraceTypeQuery TraceChannel;
	TArray<AActor*> ActorsToIgnore;
	TArray<FHitResult> OutHits;
	
	AController* LastHitBy;

	void AddGoldToEnemyPlayer();

public:
	// ----- HP 위젯 조절 함수 -----
	virtual class UWidgetComponent* GetHPWidgetComponent() const override { return WidgetComponent; }
	
	bool bLastVisibleState = true;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	float MaxVisibleDistance = 5000.f;		// 최대 가시 거리

	UPROPERTY(EditAnywhere, Category = "UI")
	float MinWidgetScale = 0.2f;

	UPROPERTY(EditAnywhere, Category = "UI")
	float MaxWidgetScale = 1.f;

	// 거리에 따라 위젯을 on/off 시키는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vision")
	class UVisibleWidgetComponent* SightComp;

	// 타겟 빔
	void BeamToTarget(FVector TargetLocation, AAOSCharacter* Target);

	// Projectile
	bool bIsSpawnedProjectile;
	void SpawnProjectile();
	float LastTime = 0.0f;

public:
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	
public:
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION(BlueprintCallable)
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION(BlueprintCallable)
	virtual void OnEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
};
