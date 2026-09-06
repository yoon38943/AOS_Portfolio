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

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TObjectPtr<UDataTable> StatTable;
	
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> InitStatEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;

	void RegisterTagEvent();

	void OnHealthChange(const FOnAttributeChangeData& Data);
	
	
public:
	UFUNCTION(Server, Reliable)
	void S_SetHPbarColor();
	UFUNCTION(NetMulticast, Reliable)
	void SetHPbarColor(FLinearColor HealthBarColor);

	UFUNCTION(Server, Reliable)
	void S_SetDamaged();
	UFUNCTION(NetMulticast, Reliable)
	void NM_SetDamaged();

	UFUNCTION(NetMulticast, Reliable)
	void TowerDestroyMulticast();
	
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
	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;
	TArray<FHitResult> OutHits;
	UPROPERTY()
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
