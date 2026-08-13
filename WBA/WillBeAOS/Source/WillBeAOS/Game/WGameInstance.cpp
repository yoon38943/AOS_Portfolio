#include "WGameInstance.h"
#include "Character/WPlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystemUtils.h"
#include "AbilitySystemGlobals.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Network/WNetStatics.h"
#ifndef WITH_GAMELIFT
#define WITH_GAMELIFT 0
#endif
#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#include "Async/Async.h"
#endif

void UWGameInstance::Init()
{
	Super::Init();

	UAbilitySystemGlobals::Get().InitGlobalData();

#if WITH_GAMELIFT
    if (IsRunningDedicatedServer())
    {
       FGameLiftServerSDKModule* GameLiftModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));
       GameLiftModule->InitSDK();

       // 1. 파라미터 구조체 생성 및 ★초기화 (쓰레기값 방지)
       FProcessParameters Params;
       FMemory::Memzero(&Params, sizeof(FProcessParameters)); 

       // 2. 포트 설정 먼저 (가장 확실한 값)
       Params.port = 7777;

       // 3. 헬스 체크 콜백 (가장 단순한 형태 유지)
       Params.OnHealthCheck.BindLambda([]() -> bool { return true; });

       // 4. 프로세스 종료 콜백 (캡처 간소화)
       Params.OnTerminate.BindLambda([]()
       {
          AsyncTask(ENamedThreads::GameThread, []()
          {
             UE_LOG(LogTemp, Warning, TEXT("GameLift: OnTerminate received! Shutting down..."));
             FGameLiftServerSDKModule* Module = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));
             Module->ProcessEnding();
             FGenericPlatformMisc::RequestExit(false);
          });
       });

       // 5. 방 생성 콜백 (★ this 캡처를 TWeakObjectPtr로 안전하게 감쌈)
       TWeakObjectPtr<UWGameInstance> WeakThis(this);
       Params.OnStartGameSession.BindLambda([WeakThis](Aws::GameLift::Server::Model::GameSession GameSession)
       {
          int32 GameLiftPort = GameSession.GetPort();
          
          AsyncTask(ENamedThreads::GameThread, [WeakThis, GameLiftPort]()
          {
             if (WeakThis.IsValid())
             {
                 UE_LOG(LogTemp, Warning, TEXT("★★★ GameLift Session Started! External Port: %d ★★★"), GameLiftPort);
                 FGameLiftServerSDKModule* Module = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));
                 Module->ActivateGameSession();
                 
                 WeakThis->SessionServerPort = GameLiftPort;
                 WeakThis->CreateSession();
             }
          });
       });

       // 6. 준비 완료 보고
       auto ReadyOutcome = GameLiftModule->ProcessReady(Params);
       if (ReadyOutcome.IsSuccess())
       {
          UE_LOG(LogTemp, Warning, TEXT("★★★★★ GameLift ProcessReady SUCCESS! Server is ready! ★★★★★"));
       }
       else
       {
       	UE_LOG(LogTemp, Error, TEXT("★★★★★ GameLift ProcessReady FAILED! Check parameters! ★★★★★"));
    
       	// 객체를 먼저 받고, 그 안의 m_errorMessage 문자열을 *를 붙여서 출력합니다!
       	FGameLiftError ProcessReadyError = ReadyOutcome.GetError();
       	UE_LOG(LogTemp, Error, TEXT("Error Details: %s"), *ProcessReadyError.m_errorMessage);
       }

       return;
    }
#endif

#if !WITH_GAMELIFT
	if (IsRunningDedicatedServer())
	{
		SessionServerPort = 7777; // 로컬 테스트용 기본 포트
		CreateSession();
	}
#endif
}

void UWGameInstance::AutoLogin()
{
	UE_LOG(LogTemp, Warning, TEXT("로그인 기록이 있는지 확인중..."));
	ClientLogin("PersistentAuth");
}

void UWGameInstance::ManualLogin()
{
	UE_LOG(LogTemp, Warning, TEXT("로그인 화면 여는중..."));
	ClientLogin("AccountPortal");
}

bool UWGameInstance::IsLoggedIn() const
{
	if (IOnlineIdentityPtr IdentityPtr = UWNetStatics::GetIdentityPtr(this))
	{
		return IdentityPtr->GetLoginStatus(0) == ELoginStatus::LoggedIn;
	}

	return false;
}

void UWGameInstance::ClientLogin(const FString& LoginType)
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	if (OSS)
	{
		// 2. 서브시스템의 진짜 이름을 로그로 찍습니다.
		FString SubsystemName = OSS->GetSubsystemName().ToString();
		UE_LOG(LogTemp, Warning, TEXT("#### 현재 활성화된 서브시스템: %s"), *SubsystemName);

		if (IOnlineIdentityPtr IdentityPtr = UWNetStatics::GetIdentityPtr(this))
		{			
			IdentityPtr->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::LoginCompleted));

			FOnlineAccountCredentials Credentials;
			Credentials.Id = TEXT("");
			Credentials.Token = TEXT("");
			Credentials.Type = LoginType;
			
			if (!IdentityPtr->Login(0, Credentials))
			{
				UE_LOG(LogTemp, Warning, TEXT("Login Failed Right Away!!!"));
				IdentityPtr->ClearOnLoginCompleteDelegates(0, this);
				OnLoginCompleted.Broadcast(false, "", "Login Failed Right Away!!!");
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("#### 온라인 서브시스템을 찾을 수 없습니다!"));
	}
}

void UWGameInstance::LoginCompleted(int NumOfLocalPlayers, bool bWasSuccessful, const FUniqueNetId& UserId,
	const FString& Error)
{
	if (IOnlineIdentityPtr IdentityPtr = UWNetStatics::GetIdentityPtr(this))
	{
		IdentityPtr->ClearOnLoginCompleteDelegates(0, this);

		FString PlayerNickname = "";
		if (bWasSuccessful)
		{
			PlayerNickname = IdentityPtr->GetPlayerNickname(UserId);
			OnLoginCompleted.Broadcast(true, PlayerNickname, "");
			UE_LOG(LogTemp, Warning, TEXT("Logged in successfully as : %s"), *PlayerNickname);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Login Failed : %s"), *Error);
			OnLoginCompleted.Broadcast(false, "", Error);
		}
	}
	else
	{
		OnLoginCompleted.Broadcast(false, "", "Can't find the Identity Pointer");
	}
}

void UWGameInstance::PlayerJoined(const FUniqueNetIdRepl& UniqueId)
{
	if (WaitPlayerJoinTimeoutHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(WaitPlayerJoinTimeoutHandle);
	}
	
	PlayerRecord.Add(UniqueId);
}

void UWGameInstance::PlayerLeft(const FUniqueNetIdRepl& UniqueId)
{
	PlayerRecord.Remove(UniqueId);
	if (PlayerRecord.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player Left is empty"));
		TerminateSessionServer();
	}
}

void UWGameInstance::JoinSessionWithSearchResult()
{
	UE_LOG(LogTemp, Warning, TEXT("Joining Session with search Result"));

	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Find Session Ptr, Cancel Joining"));
		return;
	}
	
	FName SessionNameFName = FName(NAME_GameSession);
	
	if (SessionPtr->GetNamedSession(SessionNameFName) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Session already exists. Waiting for Destroy to complete..."));

		SessionPtr->OnDestroySessionCompleteDelegates.RemoveAll(this);
		SessionPtr->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionCOmplete));
		
		if (!SessionPtr->DestroySession(SessionNameFName))
		{
			SessionPtr->OnDestroySessionCompleteDelegates.RemoveAll(this);
			InternalJoinSession();
		}
	}
	else
	{
		// 기존 세션이 없다면 즉시 Join
		InternalJoinSession();
	}
}

void UWGameInstance::OnDestroySessionCOmplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("Destroy completed. Now Joining..."));

	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	SessionPtr->ClearOnDestroySessionCompleteDelegates(this);

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		this->InternalJoinSession();
	});
}

void UWGameInstance::InternalJoinSession()
{
	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		return;
	}

	if (!SessionSearchResults.IsValidIndex(CurrentSessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid session search result found at index: %d"), CurrentSessionIndex);
		SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
		OnMatchmakingCompleted.Broadcast(false);
		OnJoinSessionFailed.Broadcast();
		return;
	}
	
	const FOnlineSessionSetting* PortSetting = SessionSearchResults[CurrentSessionIndex].Session.SessionSettings.Settings.Find(UWNetStatics::GetPortKey());
	int64 Port = 7777;
	if (PortSetting)
	{
		PortSetting->Data.GetValue(Port);
	}

	if (Port == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("유령 방(포트 0) 발견! 다음 세션으로 넘어갑니다."));
		CurrentSessionIndex++;
		
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() { this->InternalJoinSession(); });
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Trying to join session: %s, at port: %lld"), *FName(NAME_GameSession).ToString(), Port);
	SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
	SessionPtr->OnJoinSessionCompleteDelegates.AddUObject(this, &ThisClass::JoinSessionCompleted, (int)Port);
	if (!SessionPtr->JoinSession(0, NAME_GameSession, SessionSearchResults[CurrentSessionIndex]))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to join session"));
		SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
		OnMatchmakingCompleted.Broadcast(false);
		OnJoinSessionFailed.Broadcast();
	}
}

void UWGameInstance::JoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type JoinResult, int Port)
{
	if (JoinResult != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("This Session doesn't access: %d"), CurrentSessionIndex);
		CurrentSessionIndex++;
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() { this->InternalJoinSession(); });
		return;
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem) return;

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface) return;

	// 1. 접속할 IP 주소(ConnectString)를 가져옵니다. (여기엔 IP만 있고 포트는 없을 확률이 높음)
	FString ConnectString;
	SessionInterface->GetResolvedConnectString(SessionName, ConnectString);

	FString IpAddress;
	FString DummyPort;
    
	// ConnectString에 ':' 가 포함되어 있다면 쪼갭니다.
	if (ConnectString.Split(TEXT(":"), &IpAddress, &DummyPort))
	{
		// 쪼개기에 성공했다면, 순수 IP(IpAddress) 뒤에 우리가 인자로 받은 진짜 Port를 붙입니다.
		ConnectString = FString::Printf(TEXT("%s:%d"), *IpAddress, Port);
	}
	else
	{
		// 애초에 포트가 안 붙어있던 순수 IP였다면, 바로 Port를 붙여줍니다.
		ConnectString = FString::Printf(TEXT("%s:%d"), *ConnectString, Port);
	}

	UE_LOG(LogTemp, Warning, TEXT("★★★ 강제 포트 삽입 완료! 최종 접속 주소: %s ★★★"), *ConnectString);

	// 이제 제대로 된 주소+포트로 여행을 떠납니다!
	APlayerController* PC = GetFirstLocalPlayerController();
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("세션 이동 시작!"));
		PC->ClientTravel(ConnectString, TRAVEL_Absolute);
	}
}

void UWGameInstance::CreateSession()
{
	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (SessionPtr)
	{			
		FString SessionSearchId = TEXT("MyGameRoom");

		FOnlineSessionSettings OnlineSessionSettings = UWNetStatics::GenerateOnlineSessionSettings(NAME_GameSession, SessionSearchId, SessionServerPort);

		OnlineSessionSettings.Set(UWNetStatics::GetPortKey(), SessionServerPort, EOnlineDataAdvertisementType::ViaOnlineService);
		OnlineSessionSettings.Set(FName("ServerState"), FString("Ready"), EOnlineDataAdvertisementType::ViaOnlineService);
		OnlineSessionSettings.Set(FName("GameVersion"), CurrentGameVersion, EOnlineDataAdvertisementType::ViaOnlineService);
		
		SessionPtr->OnCreateSessionCompleteDelegates.RemoveAll(this);
		SessionPtr->OnCreateSessionCompleteDelegates.AddUObject(this, &ThisClass::OnSessionCreated);
		if (!SessionPtr->CreateSession(0, NAME_GameSession, OnlineSessionSettings))
		{
			UE_LOG(LogTemp, Warning, TEXT("Session Creating Failed Right Away!!!"));
			SessionPtr->OnCreateSessionCompleteDelegates.RemoveAll(this);
			TerminateSessionServer();
		}
		
		UE_LOG(LogTemp, Warning, TEXT("#### Create Session With Name: %s, ID: %s, Port: %d"), *FName(NAME_GameSession).ToString(), *SessionSearchId, SessionServerPort);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find session ptr, Terminate Session Server"));
		TerminateSessionServer();
	}
}

void UWGameInstance::StartGlobalSessionSearch()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Global Session Search"));
	
	FindSessionCount = 1;
	FindGlobalSessions();
}

void UWGameInstance::StopAllSessionFindings()
{
	UE_LOG(LogTemp, Warning, TEXT("Stopping all session Search"));
	StopFindingCreatedSession();
	StopGlobalSessionSearch();
}

void UWGameInstance::StopFindingCreatedSession()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop Finding Created Session"));
	GetWorld()->GetTimerManager().ClearTimer(FindCreatedSessionTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(FindCreatedSessionTimeoutTimerHandle);

	if (IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr())
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
		SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
	}
}

void UWGameInstance::StopGlobalSessionSearch()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop Global Session Search"));
	
	if (GlobalSessionSearchTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GlobalSessionSearchTimerHandle);
	}
	if (GlobalSessionSearchTimeoutTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GlobalSessionSearchTimeoutTimerHandle);
	}

	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (SessionPtr)
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
}

void UWGameInstance::StopGlobalSessionTimeOut()
{
	UE_LOG(LogTemp, Warning, TEXT("Global Session Timeout Reached"));
	StopGlobalSessionSearch();
	OnMatchmakingCompleted.Broadcast(false);
}

void UWGameInstance::FindGlobalSessions()
{
	UE_LOG(LogTemp, Warning, TEXT("------ Retrying Global Session Search ------"));

	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find session interface, wait for the next Global Session Search"));
		return;
	}

	SessionSearchResults.Empty();

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(FName("ServerState"), FString("Ready"), EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(FName("GameVersion"), CurrentGameVersion, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(FName(TEXT("MinSlotsAvailable")), 1, EOnlineComparisonOp::GreaterThanEquals);
	SessionSearch->MaxSearchResults = 10;

	CurrentSessionIndex = 0;
	SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	SessionPtr->OnFindSessionsCompleteDelegates.AddUObject(this, &ThisClass::GlobalSessionSearchCompleted);
	if (!SessionPtr->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Session Search Failed"));
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
}

void UWGameInstance::GlobalSessionSearchCompleted(bool bWasSuccessful)
{
	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (SessionPtr)
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
	
	if (bWasSuccessful && SessionSearch->SearchResults.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("방 찾기 성공! 접속 시도..."));
		GetWorld()->GetTimerManager().ClearTimer(GlobalSessionSearchTimeoutTimerHandle);
		OnMatchmakingCompleted.Broadcast(true);
		StopGlobalSessionSearch();
		SessionSearchResults = SessionSearch->SearchResults;
		JoinSessionWithSearchResult();
	}
	else
	{
		if (FindSessionCount >= 5)
		{
			StopGlobalSessionTimeOut();
			return;
		}
		
		UE_LOG(LogTemp, Warning, TEXT("방이 없습니다. 3초 뒤에 재검색합니다..."));

		FindSessionCount++;

		GetWorld()->GetTimerManager().SetTimer(
		   GlobalSessionSearchTimerHandle, 
		   this, 
		   &ThisClass::FindGlobalSessions, 
		   3.0f,
		   false
	   );
	}
}

void UWGameInstance::OnSessionCreated(FName SessionName, bool bSuccess)
{
	if (IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr())
	{
		SessionPtr->OnCreateSessionCompleteDelegates.RemoveAll(this);
	}
	
	 if (bSuccess)
	 {
		UE_LOG(LogTemp, Warning, TEXT("세션 생성 성공!!!"));
	 	LoadLevelAndListen(LobbyLevel);
	 }
	 else
	 {
		UE_LOG(LogTemp, Warning, TEXT("Session Creation Failed"));
		 TerminateSessionServer();
	 }
}

void UWGameInstance::EndSessionComplete(FName SessionName, bool bSuccess)
{
	FGenericPlatformMisc::RequestExit(false);
}

void UWGameInstance::LeaveSession()
{
	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (SessionPtr)
	{
		SessionPtr->OnDestroySessionCompleteDelegates.RemoveAll(this);
		SessionPtr->OnDestroySessionCompleteDelegates.AddUObject(this, &ThisClass::LeaveSessionCompleted);
		
		SessionPtr->DestroySession(NAME_GameSession);
	}
}

void UWGameInstance::LeaveSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (SessionPtr.IsValid())
	{
		SessionPtr->ClearOnDestroySessionCompleteDelegates(this);
	}

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully left the session and returning to Main Menu."));
		
		GetFirstLocalPlayerController(GetWorld())->ClientTravel(MainMenuLevel.ToString(), TRAVEL_Absolute);
	});
}

void UWGameInstance::TerminateSessionServer()
{
	if (IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr())
	{
		SessionPtr->OnEndSessionCompleteDelegates.RemoveAll(this);
		SessionPtr->OnEndSessionCompleteDelegates.AddUObject(this, &ThisClass::EndSessionComplete);
		if (!SessionPtr->EndSession(NAME_GameSession))
		{
			FGenericPlatformMisc::RequestExit(false);
		}
	}
	else
	{
		FGenericPlatformMisc::RequestExit(false);
	}
}

void UWGameInstance::WiatPlayerJoinTimeOutReached()
{
	UE_LOG(LogTemp, Warning, TEXT("Session Server shut down after %f seconds without player joining"), WaitPlayerJoinTimeOutDuration);
	TerminateSessionServer();
}

void UWGameInstance::LoadLevelAndListen(TSoftObjectPtr<UWorld> Level)
{
	const FName LevelURL = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));

	if (LevelURL != "")
	{
		FString TravelStr = FString::Printf(TEXT("%s?listen?port=%d"), *LevelURL.ToString(), SessionServerPort);
		UE_LOG(LogTemp, Warning, TEXT("Server Traveling to : %s"), *(TravelStr));
		GetWorld()->ServerTravel(TravelStr);
	}
}

void UWGameInstance::LogFinalTeamNum()
{
	UE_LOG(LogTemp, Warning, TEXT("게임 인스턴스 - Blue : %d, Red : %d"), FinalBlueTeamPlayersNum, FinalRedTeamPlayersNum);
}

void UWGameInstance::SavePlayerTeamInfo(FString& PlayerNameInfo, FPlayerInfoStruct PlayerInfo)
{
	MatchPlayersTeamInfo.Add(*PlayerNameInfo, PlayerInfo);
	UE_LOG(LogTemp, Warning, TEXT("저장된 플레이어: %s, 팀 : %s, 팀 ID: %d"), *PlayerNameInfo, PlayerInfo.PlayerTeam == E_TeamID::Blue ? TEXT("블루팀") : TEXT("레드팀"), PlayerInfo.PlayerTeamID);
}

void UWGameInstance::DeletePlayerTeamInfo(FString PlayerName)
{
	SortPlayerTeamInfo(PlayerName);
	MatchPlayersTeamInfo.Remove(PlayerName);
}

void UWGameInstance::SortPlayerTeamInfo(FString ExitingPlayerName)
{
	if (MatchPlayersTeamInfo.Contains(ExitingPlayerName))
	{
		int32 ExitingPlayerTeamID = MatchPlayersTeamInfo.Find(ExitingPlayerName)->PlayerTeamID;

		for (TPair<FString, FPlayerInfoStruct>& PlayerInfo : MatchPlayersTeamInfo)
		{
			if (PlayerInfo.Value.PlayerTeamID > ExitingPlayerTeamID)
			{
				PlayerInfo.Value.PlayerTeamID -= 1;
			}
		}
	}
}

TMap<FString, FPlayerInfoStruct> UWGameInstance::GetSavedPlayerTeamInfo()
{
	return MatchPlayersTeamInfo;
}

void UWGameInstance::AssignPlayerNickName()
{
	for (TPair<FString, FPlayerInfoStruct>& PlayerInfo : MatchPlayersTeamInfo)
	{
		if (PlayerInfo.Value.PlayerNickName.IsEmpty())
		{
			PlayerInfo.Value.PlayerNickName = FString::Printf(TEXT("%s팀 유저 %d"), PlayerInfo.Value.PlayerTeam == E_TeamID::Blue ? TEXT("블루") : TEXT("레드"), PlayerInfo.Value.PlayerTeamID + 1);
		}
	}
}