#pragma once

#include "CoreMinimal.h"
#include "AOSCharacter.h"
#include "WEnumFile.h"
#include "Character/Skill/SkillInterface.h"
#include "Character/Skill/SkillType.h"
#include "Interface/VisibleSightInterface.h"
#include "Struct_Enum/WalkSpeedStruct.h"
#include "AbilitySystemInterface.h"
#include "GAS/UWGameplayAbilityTypes.h"
#include "Interface/Interface_CharacterAction.h"
#include "WCharacterBase.generated.h"

#define PLAYERKILLGOLD 100

class UGameplayEffect;
class AWPlayerState;
struct FGameplayTag;
class UGameplayAbility;
class UWCharAnimInstance;
class AGamePlayerState;
class UWAbilitySystemComponent; // forward-declare our custom ASC
class UWAttributeSet; // forward-declare our custom AttributeSet

class AWolf;
struct FInputActionValue;
class UInputAction;
class UAnimMontage;
class UWidgetComponent;

UCLASS()
class WILLBEAOS_API AWCharacterBase : public AAOSCharacter, public ISkillInterface, public IVisibleSightInterface,
public IAbilitySystemInterface, public IInterface_CharacterAction
{

	GENERATED_BODY()

public:
	AWCharacterBase();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;


	bool bIsCombat;
	void RegisterTagEvent();
	UFUNCTION()
	void OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount);
	
protected:
	//컴포넌트
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"));
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"));
	class UCameraComponent* FollowCamera;
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"));
	class UCombatComponent* CombatComp;

public:
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
public:
	UPROPERTY()
	AGamePlayerState* GamePlayerState;

	UWAbilitySystemComponent* GetGameplayerStateASC() const;
	
	void SetTeamCollision();
	
	virtual void RequestSnapToCameraDirection() override;
	

public:
	
	// HP Widget 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UWidgetComponent* HPInfoBarComponent;

	float BaseWidgetHeight = 160.f;

	UPROPERTY(EditAnywhere, Category = "Color")
	FLinearColor BlueTeamHPColor;
	UPROPERTY(EditAnywhere, Category = "Color")
	FLinearColor RedTeamHPColor;
	UPROPERTY(EditAnywhere, Category = "Color")
	FLinearColor SelfHPColor;
	
	FLinearColor HPInfoBarColor;

	FTimerHandle HealingTimerHandle;
	
	virtual void SetHPInfoBarColor();

	void ConfigureOverHeadHealthWidget();

	virtual void ShowNickName();

	// 거리에 따라 위젯을 on/off 시키는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vision")
	class UVisibleWidgetComponent* SightComp;

	// 플레이어 거리 계산
	FTimerHandle CheckTimerHandle;

	virtual UWidgetComponent* GetHPWidgetComponent() const override { return HPInfoBarComponent; }

private: 
	//입력 에셋
	UPROPERTY(EditAnywhere, Category = Input)
	class UInputMappingContext* IMC_Asset;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* IA_Look;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* IA_Move;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* IA_BasicAttack;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* IA_SkillQ;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* IA_SkillE;
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* IA_Recall;

	UPROPERTY(EditAnywhere, Category = Input)
	TMap<EWAbilityInputID, UInputAction*> GameplayAbilityInputActions;


public:
	/*********************************************************/
	// GAS 시스템
	/*********************************************************/

	void InitialAbilitySystem();
	void ServerSideInit();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

private:
	UPROPERTY()
	UWAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TSubclassOf<UGameplayEffect> InitStatEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Stat")
	TObjectPtr<UDataTable> StatTable;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<EWAbilityInputID, TSubclassOf<UGameplayAbility>> Abilities;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<EWAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;

public:
	/*********************************************************/
	// Skill Ability / 스킬 어빌리티
	/*********************************************************/
	
	void HandleAbilityInputPressed(const FInputActionValue& Value, EWAbilityInputID InputID);
	void HandleAbilityInputReleased(const FInputActionValue& Value, EWAbilityInputID InputID);

	UPROPERTY()
	UDecalComponent* SkillForwardDecal;

	void MoveDecalToCameraForward();
	
public:
	UPROPERTY(BlueprintReadOnly)
	UWCharAnimInstance* Anim;
	UPROPERTY(BlueprintReadWrite, Category = "Health")
	UAnimMontage* DeadAnimMontage;
	UPROPERTY(BlueprintReadWrite, Category = "Health")
	UAnimMontage* HitAnimMontage;
	UPROPERTY(BlueprintReadWrite, Category = Combo)
	TArray<UAnimMontage*> AttackMontages = {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skill)
	UDataTable* SkillDataTable;

	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	virtual void StopMove(const FInputActionValue& Value);
	void VisibleOutline();

	FMovementSpeedStruct MovementSpeedData;
	
	UFUNCTION(BlueprintCallable)
	void UpdateMovementSpeedData(float Multiplier);
	void UpdateAcceleration();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetControlRotationYaw(FRotator YawRotation);
	
	// ---- 귀환 관련 함수 ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recall")
	TObjectPtr<UParticleSystem> RecallParticle;

	void RecallAbilityInputPressed(const FInputActionValue& Value, TSubclassOf<UGameplayAbility> AbilityClass);

	UPROPERTY(EditAnywhere, Category = "Recall")
	UAnimMontage* StartRecallMontage;
	UPROPERTY(EditAnywhere, Category = "Recall")
	UAnimMontage* CompleteRecallMontage;

	UAnimMontage* GetStartRecallMontage() const { return StartRecallMontage; }
	UAnimMontage* GetCompleteRecallMontage() const { return CompleteRecallMontage; }
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiPlayMontage(UAnimMontage* Montage);

	// ---- 타겟 관리 함수 ----
	UPROPERTY()
	TArray<AActor*> AttackTarget;

	UPROPERTY()
	TArray<AActor*> CurrentTarget;
	
	// ---- Attack 관련 함수 ----
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool IsCombat = false;

	UFUNCTION(Server, Reliable)
	void Server_EnterCombat();
	void ServerExitCombat();

	virtual void ServerChangeCombatMode(bool isCombat);

	FTimerHandle CombatTimer;

	// ----- Hit 이벤트 -----
	UFUNCTION(BlueprintNativeEvent)
	void SpawnHitEffect(FVector HitLocation);
	UFUNCTION(NetMulticast, Reliable)
	void NM_SpawnHitEffect(FVector HitLocation);

	// ----- 스킬 이벤트 관련 -----

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_QSkillUsing)
	bool bIsQSkillUsing = false;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ESkillUsing)
	bool bIsESkillUsing = false;

	UFUNCTION()
	virtual void OnRep_QSkillUsing();
	UFUNCTION()
	virtual void OnRep_ESkillUsing();

	void Input_QSkill(const FInputActionValue& Value);
	void Input_ESkill(const FInputActionValue& Value);
	void Input_RSkill(const FInputActionValue& Value);
	
	virtual void Handle_UseSkillButton(ESkillSlot Skillslot);
	
	// PlayerState에게 요청 함수
	virtual void ActivateSkill_Implementation(ESkillSlot SkillSlot) override;

	// 캐릭터에서 스킬 실행 함수
	virtual void ExecuteSkill(ESkillSlot SkillSlot);

	// ---- 골드 관련 ----
	virtual void SetGoldReward(int32 NewGold) override {GoldReward = NewGold;}
	
	// ---- Dead 관련 함수 -----
	UFUNCTION(Server, Reliable)
	void S_BeingDead(class AGamePlayerController* PC, APawn* Player);
	// 일반 죽는 함수(죽는 클라이언트 본인만 실행되도록)
	void BeingDead();
	// 클라이언트
	UFUNCTION(Client, Reliable)
	void C_BeingDead(AGamePlayerController* PC);

	// 델리게이트 함수
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Dead)
	void NM_BeingDead();//죽을 때 델리게이트로 호출 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HandleApplyPointDamage(FHitResult LastHit);//?�인???��?지�?줄시 ?�리게이?�로 ?�출???�수
	UFUNCTION(BlueprintCallable, Category = "Combat")//TakeDamage ?�수 ?�버?�이??
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	void ClearLastHitBy();

	//게임 엔딩
	void HandleGameEnd();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Combat")
	float CharacterDamage;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};