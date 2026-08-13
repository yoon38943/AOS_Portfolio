#pragma once

#include "CoreMinimal.h"
#include "PlayerInfoStruct.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "WGameInstance.generated.h"

enum class E_TeamID : uint8;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLoginCompleted, bool /*bWasSuccessful*/, const FString& /*UserId*/, const FString& /*Error*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchmakingCompleted, bool /*bWasSuccessful*/);
DECLARE_MULTICAST_DELEGATE(FOnJoinSessionFailed);

UCLASS()
class WILLBEAOS_API UWGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;

	int32 CurrentGameVersion = 100;

public:
	FOnMatchmakingCompleted OnMatchmakingCompleted;
	FOnJoinSessionFailed OnJoinSessionFailed;
	
/******************************************/
/*				  로그인                   */
/******************************************/
public:
	void AutoLogin();
	void ManualLogin();
	bool IsLoggedIn() const;

	FOnLoginCompleted OnLoginCompleted;
	
private:
	void ClientLogin(const FString& LoginType);
	void LoginCompleted(int NumOfLocalPlayers, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

/******************************************/
/*              세션 관리 부분              */
/******************************************/
public:
	void PlayerJoined(const FUniqueNetIdRepl& UniqueId);
	void PlayerLeft(const FUniqueNetIdRepl& UniqueId);

	void StartGlobalSessionSearch();

	void LeaveSession();
	void LeaveSessionCompleted(FName SessionName, bool bWasSuccessful);

private:
	int32 FindSessionCount = 1;
	
	void StopAllSessionFindings();
	void StopFindingCreatedSession();
	void StopGlobalSessionSearch();
	void StopGlobalSessionTimeOut();
	void FindGlobalSessions();
	void GlobalSessionSearchCompleted(bool bWasSuccessful);

	FTimerHandle FindCreatedSessionTimerHandle;
	FTimerHandle FindCreatedSessionTimeoutTimerHandle;
	FTimerHandle GlobalSessionSearchTimerHandle;
	FTimerHandle GlobalSessionSearchTimeoutTimerHandle;

	TArray<FOnlineSessionSearchResult> SessionSearchResults;

	int32 CurrentSessionIndex = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float GlobalSessionSearchInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float GlobalSessionTimeoutDuration = 15.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float FindCreatedSessionSearchInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float FindCreatedSessionTimeoutDuration = 60.f;
	
	void JoinSessionWithSearchResult();
	void OnDestroySessionCOmplete(FName SessionName, bool bWasSuccessful);
	void InternalJoinSession();
	void JoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type JoinResult, int Port);

	TSharedPtr<class FOnlineSessionSearch> Sessionsearch;
	
	void CreateSession();
	void OnSessionCreated(FName SessionName, bool bSuccess);
	void EndSessionComplete(FName SessionName, bool bSuccess);

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FString ServerSessionName;
	int SessionServerPort;

	void TerminateSessionServer();

	FTimerHandle WaitPlayerJoinTimeoutHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Sesssion")
	float WaitPlayerJoinTimeOutDuration = 60.f;

	void WiatPlayerJoinTimeOutReached();

	TSet<FUniqueNetIdRepl> PlayerRecord;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> MainMenuLevel;
	
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> GameLevel;
	
	void LoadLevelAndListen(TSoftObjectPtr<UWorld> Level);

	// 맵의 경로 지정
	UPROPERTY(EditAnywhere, Category = "Map")
	TSoftObjectPtr<UWorld> OutLobbyMap;

public:
	UPROPERTY()
	TMap<FString, FPlayerInfoStruct> MatchPlayersTeamInfo;  // PlayerName을 기반으로 팀 정보 저장

	UPROPERTY(BlueprintReadOnly)
	int32 FinalBlueTeamPlayersNum;
	UPROPERTY(BlueprintReadOnly)
	int32 FinalRedTeamPlayersNum;

	void LogFinalTeamNum();
	
public:
	// GameInstance에 Key 값 저장
	void SavePlayerTeamInfo(FString& PlayerNameInfo, FPlayerInfoStruct PlayerInfo);

	void DeletePlayerTeamInfo(FString PlayerName);

	void SortPlayerTeamInfo(FString ExitingPlayerName);
	
	TMap<FString, FPlayerInfoStruct> GetSavedPlayerTeamInfo();

	void AssignPlayerNickName();
};