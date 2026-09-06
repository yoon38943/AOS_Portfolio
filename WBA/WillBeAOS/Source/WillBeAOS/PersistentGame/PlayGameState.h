#pragma once

#include "CoreMinimal.h"
#include "WEnumFile.h"
#include "GameFramework/GameState.h"
#include "Gimmick/Nexus.h"
#include "PlayGameState.generated.h"

class AGamePlayerState;
class AGamePlayerController;
struct FPlayerInfoStruct;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDelegateShowKillState, int, BlueTeamScore, int, RedTeamScore);

UCLASS()
class WILLBEAOS_API APlayGameState : public AGameState
{
	GENERATED_BODY()

	APlayGameState();

public:
	UPROPERTY(ReplicatedUsing = OnRep_ChangeGamePhase)
	EGamePhase CurrentGamePhase = EGamePhase::StartGameWaiting;

	virtual void SetGamePhase(EGamePhase NewGamePhase);

	UFUNCTION()
	void OnRep_ChangeGamePhase();

	void EnterCharacterSelectPhase();

	void Client_EnterCharacterSelectPhase();

	void EnterLoadingPhase();

	void EnterInGamePhase();

	void Client_EnterInGamePhase();


	
	// ---------------------------------------------
	// 플레이 캐릭터 선택 게임 스테이트
	// ---------------------------------------------
public:
	
	// TMap은 Replicated가 안되기에 TArray로 교체
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_UpdateWidget)
	TArray<FPlayerInfoStruct> BlueTeamPlayerInfo;
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_UpdateWidget)
	TArray<FPlayerInfoStruct> RedTeamPlayerInfo;

	UFUNCTION()
	void OnRep_UpdateWidget();
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 BlueTeamPlayersNum;
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 RedTeamPlayersNum;

	// 캐릭터 선택 카운트 다운
	UPROPERTY(BlueprintReadOnly, Replicated)
	float SelectCountdown = 20.0;
	FTimerHandle CountdownTimerHandle;

	void UpdateCountdown();

	// 플레이어 맵 로드 완료
	UPROPERTY()
	TArray<AGamePlayerController*> ReadyPlayers;
	
	void CheckPlayerIsReady(AGamePlayerController* PC);

	void AddSelectCharacterToPlayerInfo(const FString& PlayerName, TSubclassOf<APawn>& ChosenChar, E_TeamID& Team, FName CharacterName);

	// 모든 플레이어가 캐릭터를 선택 했는지 체크
	void AllPlayerChosenChar();

	// 캐릭터 선택 공백시 모든 플레이어 로비 복귀
	void AllPlayerBackToLobby();

	// 캐릭터 선택 후 정보 인스턴스에 업로드
	void UploadStateToGameInstance();

	// 캐릭터 선택 후 정보 플레이어 스테이트에 업데이트
	void UpdateChosenCharacterToPlayerState();

	// ---------------------------------------------
	// 인게임 게임 스테이트
	// ---------------------------------------------
public:	//GameInit 단계
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	class APlayGameMode* PlayGameMode;
	
	UPROPERTY(BlueprintReadWrite, Category = "State")
	E_GamePlay CurrentInGamePhase = E_GamePlay::Nothing;	//게임진행상황을 담은 변수

	int32 GetGMTime = 0;
	FTimerHandle GetGMHandle;
	
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void UpdateGMTimer();
	
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void SetGamePlay(E_GamePlay NewState);
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void GamePlayStateChanged(E_GamePlay NewState);

	//인스턴스에서 받아온 MatchedPlayers
	TArray<FPlayerInfoStruct> MatchPlayersInfo;
	// Info 클라에게 넘겨주기
	void TakeAllMatchPlayersInfo();
	
	//접속한 플레이어 컨트롤러
	UPROPERTY(EditAnywhere)
	TArray<AGamePlayerController*> PlayerControllers;
	
	void SetGamePlayerReady();
	
	UFUNCTION(BlueprintCallable)
	bool IsAllPlayerIsReady();
	
	UPROPERTY(BlueprintReadWrite, Category = "Players")
	TArray<AGamePlayerState*> ConnectedPlayerStates;
	
	void RemovePlayer(AGamePlayerController* WPlayerController);
	
public: //PlayerReady단계
	void CheckPlayerSpawned(AGamePlayerController* WPlayerController);

	int32 CheckSpawnedPlayers = 0;

	// 모든 플레이어가 준비되었는지 확인하는 함수
	UFUNCTION()
	void CheckAllPlayersReady();

public:
	FTimerHandle InGameTimerHandle;
	
	float InGameTime = 0;
	
	void CountInGameTime();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_CountInGameTime(float Time);

public:
	int32 CountdownTime = 0;
protected:
	void ServerCountdown();
public:
	void SetCountdownTime(int32 NewCount);
	
public:
	void SetGameStart();
	
public://리스폰 관련
	UPROPERTY(BlueprintReadWrite, Replicated)
	int32 RespawnTime = 8;

	FTimerHandle RespawnTimeHandle;
	void AddRespawnTime();

public://타워 관련
	void AssignNexus(AAOSActor* SpawnedActor);

protected://넥서스
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tower")
	ANexus* BlueNexus;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tower")
	ANexus* RedNexus;

public:
	float GetBlueNexusHP();
	float GetRedNexusHP();

	// 골드 관련
	int Gold_Attack = 120;
	int Gold_Health = 150;
	int Gold_Defence = 60;
	int Gold_Speed = 60;

	// 캐릭터가 거리 계산할 인스턴스 모음
	UPROPERTY(ReplicatedUsing = OnRep_ManagedActors)
	TArray<AActor*> GameManagedActors;

	UFUNCTION()
	void OnRep_ManagedActors();

	TArray<TWeakObjectPtr<AActor>> CachedActors;	// 클라에서 사용할 !nullptr ManagedActors 모음

	// 팀별 킬 점수
	FDelegateShowKillState DelegateShowKillState;
	int32 BlueTeamTotalKillPoints;
	int32 RedTeamTotalKillPoints;

	UFUNCTION(NetMulticast, reliable)
	void NM_ReplicateTotalKillPoints(int32 Blue, int32 Red);

	void CheckKilledTeam(E_TeamID KillTeam);
	
protected:
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
