#pragma once

#include "CoreMinimal.h"
#include "PlayerInfoStruct.h"
#include "GameFramework/PlayerState.h"
#include "GamePlayerState.generated.h"

DECLARE_DELEGATE(FLoadSkillIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCooldown, FSkillUsedInfo, UsedSkillInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);

UCLASS()
class WILLBEAOS_API AGamePlayerState : public APlayerState
{
	GENERATED_BODY()

	AGamePlayerState();

protected:
	virtual void BeginPlay() override;

	// ---------------------------------------------
	// 플레이 캐릭터 선택 페이즈
	// ---------------------------------------------
public:
	void StartCharacterSelectPhase();
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	FPlayerInfoStruct PlayerInfo;

	UFUNCTION(Client, Reliable)
	void Client_PlayerInfoReady(FPlayerInfoStruct PlayerInfoStruct);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_ChooseTheCharacter(TSubclassOf<APawn> ChosenChar, FName CharacterName);

	UFUNCTION(Server, Reliable)
	void Server_ReplicatePlayerInfo(const FString& ClientPlayerName);
	

	
	// ---------------------------------------------
	// 인게임 플레이 페이즈
	// ---------------------------------------------
	public:
	void StartInGamePhase();
	
	UPROPERTY(Replicated)
	bool bIsGameReady = false;

	UFUNCTION(Server, Reliable, WithValidation)
	void S_SetPlayerReady(bool bReady);

	// 유저 정보
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AddSkillIcon)
	FPlayerInfoStruct InGamePlayerInfo;

	FLoadSkillIcon LoadSkillIcon;
	
	UFUNCTION()
	void OnRep_AddSkillIcon();

	// 유저 정보 받아오기
	UFUNCTION(Server, Reliable)
	void Server_TakePlayerInfo(const FString& PlayerName);

public:
	UPROPERTY()
	class APlayerSpawner* PlayerSpawner;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	UWAbilitySystemComponent* WAbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	UWAttributeSet* WAttributeSet;
	
	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float HP;
	UPROPERTY(Replicated)
	float MaxHP;

	UFUNCTION()
	void OnRep_Health();
	
public:
	UWAbilitySystemComponent* GetAbilitySystemComponent() const { return WAbilitySystemComponent; }
	
	FOnHealthChanged OnHealthChanged;
	
	UFUNCTION(BlueprintPure)
	float GetHP();
	UFUNCTION(BlueprintPure)
	float GetMaxHP();
	UFUNCTION(BlueprintCallable)
	void SetHP(int32 NewHP);

	UFUNCTION(BlueprintPure)
	float GetHPPercentage();
	
    // Attack power
    UPROPERTY(EditDefaultsOnly, Category = "Stats")
    TSubclassOf<UGameplayEffect> AddAttackEffectClass;
	UFUNCTION(Server, Reliable)
	void AddPower(int32 Power);
    // Additional health
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> AddHealthEffectClass;
	UFUNCTION(Server, Reliable)
	void AddHealth(int32 Health);
    // Defense power
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> AddDefenseEffectClass;
	UFUNCTION(Server, Reliable)
	void AddDefence(float Defence);
    // Movement speed
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> AddSpeedEffectClass;
	UFUNCTION(Server, Reliable)
	void AddSpeed(float Speed);
    //Exp
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 CCurrentExp;
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 CExperience;
    // Level
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 CLevel;
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 CAbLevel;

	// ----- 스킬 관련 -----
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSkillCooldown OnSkillCooldown;

	float LastUseQSkillTime = -100.f;
	float LastUseESkillTime = -100.f;

	// 스킬 실행할 캐릭터가 "요청"하는 함수
	UFUNCTION(Server, Reliable)
	void Server_RequestUseSkill(ESkillSlot SkillSlot);

protected:
	UPROPERTY(Replicated)
	TArray<FSkillUsedInfo> SkillCooldownEndTimes;

	UPROPERTY()
	FSkillUsedInfo LastSkillCooldown;

	UFUNCTION(Client, Reliable)
	void Client_Delegate_UsedSkill(FSkillUsedInfo LastUsedSkill);

	// 쿨다운 검사 및 시작을 위한 헬퍼 함수
	bool IsSkillReady(ESkillSlot Skillslot);
	void StartCooldown(ESkillSlot Skillslot);

	

public:
	// ----- 플레이어 골드 관련 스탯 -----
	// Gold
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gold")
	int32 CGold;

	// 서버에서 골드를 추가하는 함수
	UFUNCTION(Server, Reliable)
	void Server_AddGold(int Amount);
	UFUNCTION(Client, Reliable)
	void C_AddGold(int NewGold);

protected:
	// 플레이어 게임 정보
	UPROPERTY(BlueprintReadOnly)
	int32 PlayerKillCount;
	UPROPERTY(BlueprintReadOnly)
	int32 PlayerDeathCount;
	UPROPERTY(BlueprintReadOnly)
	float PlayerDamageAmount;

public:
	// Replicated가 너무 느려서 RPC로 변경
	UFUNCTION(NetMulticast, Reliable)
	void AddDeathPoint();
	UFUNCTION(NetMulticast, Reliable)
	void AddKillPoint();
	int32 GetKillPoints();
	int32 GetDeathPoints();
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
