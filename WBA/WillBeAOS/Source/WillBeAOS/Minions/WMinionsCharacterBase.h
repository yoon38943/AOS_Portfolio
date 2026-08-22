#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Character/AOSCharacter.h"
#include "Interface/VisibleSightInterface.h"
#include "WMinionsCharacterBase.generated.h"

#define MINIONKILLGOLD 30

class UGameplayEffect;
class UAbilitySystemComponent;
class UWAttributeSet;
class UWAbilitySystemComponent;
class AGamePlayerController;
class AWCharacterBase;
class UAnimMontage;
class UCombatComponent;
class UWidgetComponent;
class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDetectedSignature, AActor*, DetectedActor);

UCLASS()
class WILLBEAOS_API AWMinionsCharacterBase : public AAOSCharacter, public IVisibleSightInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCombatComponent* CombatComponent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UWidgetComponent* WidgetComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USphereComponent* PerceptionCollision;

public:
	AWMinionsCharacterBase();

	void SetTeamCollision();
	
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
	TSubclassOf<UGameplayEffect> InitStatEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Stat")
	TObjectPtr<UDataTable> StatTable;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;

public:
	// ----- HP 위젯 조절 함수 -----
	bool bLastVisibleState = true;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	float MaxVisibleDistance = 5000.f;		// 최대 가시 거리

	UPROPERTY(EditAnywhere, Category = "UI")
	float MinWidgetScale = 0.2f;

	UPROPERTY(EditAnywhere, Category = "UI")
	float MaxWidgetScale = 1.f;
	
	AWCharacterBase* PlayerChar;
	AGamePlayerController* PlayerController;
	FTimerHandle MinionPCTimerManager;
	
	UFUNCTION()
	void FindPlayerPC();
	void FindPlayerPawn();

	// 거리에 따라 위젯을 on/off 시키는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vision")
	class UVisibleWidgetComponent* SightComp;

	virtual class UWidgetComponent* GetHPWidgetComponent() const override { return WidgetComponent;}
	
	// 타겟 인식 관련 로직
	FTimerHandle CheckDistanceTimerHandle;

	float VisibleWidgetDistance = 1200.f;

	TSet<AActor*> CurrentObserveObjects;

	void CheckDistanceToTarget();

	UFUNCTION(BlueprintNativeEvent)
	void CanAttackToTarget(AActor* WObject);
	UFUNCTION(BlueprintNativeEvent)
	void DetachToTarget(AActor* WObject);

public://트랙 관련
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadOnly, Category = "Track")
	int32 TrackNum;

	UFUNCTION(BlueprintNativeEvent, Category = "Track")
	void SetTrackPoint();//트랙 정하는 이벤트
	void SetTrackPoint_Implementation(){}
	
public:	//골드 관련
	virtual void SetGoldReward(int32 NewGold){GoldReward = NewGold;}
	
public:	//타격 관련
	AController* LastHitBy;		// 마지막 타격 주체 저장
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* MinionAttackMontage;

	UFUNCTION(NetMulticast,BlueprintCallable, Reliable)
	void NM_Minion_Attack();

public:	//체력관련
	FTimerHandle HPbarColorTimerHandle;
	
	void StartSetHPbarColor();
	void SetHPbarColor(FLinearColor HealthBarColor);

public: //죽을 때
	UPROPERTY(BlueprintReadWrite, Category = Dead)
	UAnimMontage* DeadAnimMontage;
	UFUNCTION()
	void Dead();
	UFUNCTION(NetMulticast, Reliable)
	void NM_BeingDead();

	// 게임 엔딩
	void HandleGameEnd();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
