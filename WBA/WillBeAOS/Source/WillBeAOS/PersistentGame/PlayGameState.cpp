#include "PersistentGame/PlayGameState.h"

#include "GamePlayerController.h"
#include "GamePlayerState.h"
#include "PlayGameMode.h"
#include "Game/WGameInstance.h"
#include "Gimmick/Nexus.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


APlayGameState::APlayGameState()
{
	bReplicates = true;
	SetNetUpdateFrequency(100.f);
}

void APlayGameState::SetGamePhase(EGamePhase NewGamePhase)
{
	if (HasAuthority() && CurrentGamePhase != NewGamePhase)
	{
		CurrentGamePhase = NewGamePhase;

		switch (CurrentGamePhase)
		{
		case EGamePhase::CharacterSelect:
			EnterCharacterSelectPhase();
			break;
		case EGamePhase::LoadingPhase:
			EnterLoadingPhase();
			break;
		case EGamePhase::InGame:
			EnterInGamePhase();
			break;
		}
	}
}

void APlayGameState::OnRep_ChangeGamePhase()
{
	if (!HasAuthority())
	{
		switch (CurrentGamePhase)
		{
		case EGamePhase::CharacterSelect:
			{
				FTimerHandle TimerHandle;
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::Client_EnterCharacterSelectPhase, 0.05f, false);
				break;
			}
		case EGamePhase::InGame:
			Client_EnterInGamePhase();
			break;
		}
	}
}

void APlayGameState::EnterCharacterSelectPhase()
{
	if (!HasAuthority()) return;
	
	UWGameInstance* GI = Cast<UWGameInstance>(GetGameInstance());
	if (GI)
	{
		for (auto& Elem : GI->MatchPlayersTeamInfo)
		{
			if (Elem.Value.PlayerTeam == E_TeamID::Blue)
			{
				BlueTeamPlayerInfo.Add(Elem.Value);
			}
			else if (Elem.Value.PlayerTeam == E_TeamID::Red)
			{
				RedTeamPlayerInfo.Add(Elem.Value);
			}
		}
		
		BlueTeamPlayersNum = GI->FinalBlueTeamPlayersNum;
		RedTeamPlayersNum = GI->FinalRedTeamPlayersNum;

		UE_LOG(LogTemp, Log, TEXT("게임 인스턴스 -> 게임 스테이트 정보 받아오기 완료!!!(Select Character Map)"));
	}
}

void APlayGameState::Client_EnterCharacterSelectPhase()
{
	AGamePlayerController* PC = Cast<AGamePlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (PC)
	{
		PC->StartCharacterSelectPhase();

		AGamePlayerState* PS = PC->GetPlayerState<AGamePlayerState>();
		if (PS)
		{
			PS->StartCharacterSelectPhase();
		}
	}
}

void APlayGameState::EnterLoadingPhase()
{
	// 맵 전환 로딩창 띄우기
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AGamePlayerController* PC = Cast<AGamePlayerController>(Iterator->Get());
		if (PC)
		{
			PC->ToInGameLoading();
		}
	}
}

void APlayGameState::EnterInGamePhase()
{
	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTimeHandle, this, &ThisClass::AddRespawnTime, 300.f, true);
	}

	PlayGameMode = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (PlayGameMode)
	{
		SetGamePlay(E_GamePlay::GameInit);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(GetGMHandle, this, &ThisClass::UpdateGMTimer, 0.1f, true);
	}
}

void APlayGameState::Client_EnterInGamePhase()
{
	AGamePlayerController* PC = Cast<AGamePlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC)
	{
		PC->StartInGamePhase();

		AGamePlayerState* PS = PC->GetPlayerState<AGamePlayerState>();
		if (PS)
		{
			PS->StartInGamePhase();
		}
	}
}

void APlayGameState::OnRep_UpdateWidget()
{
	AGamePlayerController* PC = Cast<AGamePlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC && PC->SelectCharacterWidget)
	{
		PC->UpdatePlayerWidget();
	}
}

void APlayGameState::UpdateCountdown()
{
	SelectCountdown--;

	if (SelectCountdown <= 0 && CountdownTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

		FTimerHandle CheckTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(CheckTimerHandle, this, &ThisClass::AllPlayerChosenChar, 1.f, false);
	}
}

void APlayGameState::CheckPlayerIsReady(AGamePlayerController* PC)
{
	ReadyPlayers.Add(PC);

	if (ReadyPlayers.Num() == PlayerArray.Num())
	{
		UE_LOG(LogTemp, Log, TEXT("모든 플레이어 맵 로드 완료(SelectChar Map)"));

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AGamePlayerController* PlayerController = Cast<AGamePlayerController>(It->Get());
			if (PlayerController)
			{
				PlayerController->Client_StartSelectCharacter();
			}
		}
		
		GetWorld()->GetTimerManager().SetTimer(CountdownTimerHandle, this, &ThisClass::UpdateCountdown, 1.f, true);
	}
}

void APlayGameState::AddSelectCharacterToPlayerInfo(const FString& PlayerName, TSubclassOf<APawn>& ChosenChar, E_TeamID& Team, FName CharacterName)
{
	if (Team == E_TeamID::Blue)
	{
		for (auto& Elem : BlueTeamPlayerInfo)
		{
			if (Elem.PlayerName == PlayerName)
			{
				Elem.SelectedCharacter = ChosenChar;
				Elem.SelectedCharacterName = CharacterName;
			}
		}
	}
	else if (Team == E_TeamID::Red)
	{
		for (auto& Elem : RedTeamPlayerInfo)
		{
			if (Elem.PlayerName == PlayerName)
			{
				Elem.SelectedCharacter = ChosenChar;
				Elem.SelectedCharacterName = CharacterName;
			}
		}
	}
}

void APlayGameState::AllPlayerChosenChar()
{
	for (auto& BlueTeam : BlueTeamPlayerInfo)
	{
		if (BlueTeam.SelectedCharacter == nullptr)
		{
			AllPlayerBackToLobby();
			return;
		}
	}

	for (auto& RedTeam : RedTeamPlayerInfo)
	{
		if (RedTeam.SelectedCharacter == nullptr)
		{
			AllPlayerBackToLobby();
			return;
		}
	}

	UpdateChosenCharacterToPlayerState();
	UploadStateToGameInstance();
}

void APlayGameState::AllPlayerBackToLobby()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AGamePlayerController* PC = Cast<AGamePlayerController>(Iterator->Get());
		if (PC)
		{
			PC->BackToLobby();
		}
	}
}

void APlayGameState::UploadStateToGameInstance()
{
	// 인스턴스에 정보 업로드
	UE_LOG(LogTemp, Log, TEXT("인스턴스에 캐릭터 선택 정보 업로드..."));
	
	UWGameInstance* GI = Cast<UWGameInstance>(GetGameInstance());
	if (GI)
	{
		for (auto& Elem : GI->MatchPlayersTeamInfo)
		{
			for (auto& BlueTeam : BlueTeamPlayerInfo)
			{
				if (Elem.Key == BlueTeam.PlayerName)
				{
					Elem.Value.SelectedCharacter = BlueTeam.SelectedCharacter;
				}
			}

			for (auto& RedTeam : RedTeamPlayerInfo)
			{
				if (Elem.Key == RedTeam.PlayerName)
				{
					Elem.Value.SelectedCharacter = RedTeam.SelectedCharacter;
				}
			}
		}
	}

	APlayGameMode* GM = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->StartLoading();
	}
}

void APlayGameState::UpdateChosenCharacterToPlayerState()
{
	// 캐릭터 선택 시간이 끝났을 때 PlayerState에 선택한 캐릭터 업데이트
	if (HasAuthority())
	{
		for (APlayerState* Player : PlayerArray)
		{
			AGamePlayerState* PS = Cast<AGamePlayerState>(Player);
			if (PS)
			{
				for (auto& Elem : BlueTeamPlayerInfo)
				{
					if (PS->GetPlayerName() == Elem.PlayerName)
					{
						PS->PlayerInfo.SelectedCharacter = Elem.SelectedCharacter;
						return;
					}
				}

				for (auto& Elem : RedTeamPlayerInfo)
				{
					if (PS->GetPlayerName() == Elem.PlayerName)
					{
						PS->PlayerInfo.SelectedCharacter = Elem.SelectedCharacter;
						return;
					}
				}
			}
		}
	}
}

void APlayGameState::UpdateGMTimer()
{
	PlayGameMode = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (PlayGameMode)
	{
		GetWorldTimerManager().ClearTimer(GetGMHandle);
	}
}

void APlayGameState::SetGamePlay(E_GamePlay NewState)
{
	if (CurrentInGamePhase == NewState) {return;}
    
	CurrentInGamePhase = NewState;
	UE_LOG(LogTemp, Log, TEXT("InGamePhase : %s"), *UEnum::GetValueAsString(CurrentInGamePhase));

	GamePlayStateChanged(CurrentInGamePhase);
}

void APlayGameState::GamePlayStateChanged(E_GamePlay NewState)
{
	switch (NewState)
	{
	case E_GamePlay::GameInit:
		UE_LOG(LogTemp, Log, TEXT("Game is initializing..."));
		TakeAllMatchPlayersInfo();
		break;

	case E_GamePlay::PlayerReady:	//플레이어 컨트롤러 정보를 다 받아와서
		UE_LOG(LogTemp, Log, TEXT("All Players are ready! Updating PlayerControllers..."));
		GetWorld()->GetTimerManager().SetTimer(InGameTimerHandle, this, &ThisClass::CountInGameTime, 1.f, true);
		break;

	case E_GamePlay::ReadyCountdown:
		UE_LOG(LogTemp, Log, TEXT("Countdown before game starts!"));
		ServerCountdown();
		break;

	case E_GamePlay::Gameplaying:
		UE_LOG(LogTemp, Log, TEXT("Game has started!"));
		SetGameStart();
		break;

	case E_GamePlay::GameEnded:		//넥서스 파괴 후
		UE_LOG(LogTemp, Log, TEXT("Game has ended!"));
		break;
	}
}

void APlayGameState::TakeAllMatchPlayersInfo()
{
	if (!HasAuthority()) return;

	// 블루팀 플레이어 담기
	for (auto& Player : BlueTeamPlayerInfo)
	{
		MatchPlayersInfo.Add(Player);
	}

	// 레드팀 플레이어 담기
	for (auto& Player : RedTeamPlayerInfo)
	{
		MatchPlayersInfo.Add(Player);
	}

	UE_LOG(LogTemp, Log, TEXT("게임 스테이트에 매치 플레이어 정보 넘기기 완료!(InGameMap)"));
}

void APlayGameState::SetGamePlayerReady()
{
	if (IsAllPlayerIsReady())
	{
		SetGamePlay(E_GamePlay::PlayerReady);
	}
}

bool APlayGameState::IsAllPlayerIsReady()
{
	TArray<AGamePlayerState*> AllPlayerStates;

	int32 NumPlayers = 0;
    
	for (auto& It : PlayerControllers)
	{
		if (AGamePlayerState* PS = Cast<AGamePlayerState>(It->PlayerState))
		{
			AllPlayerStates.Add(PS);
			UE_LOG(LogTemp, Log, TEXT("GameState PlayerStateAdd %s"), *PS->GetPlayerName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerState not found or cast failed."));
		}
	}
    
	for (auto PS : AllPlayerStates)
	{
		for (auto It : MatchPlayersInfo)
		{
			if (PS)// 특정 이름 패턴을 가진 경우만 처리
			{
				if (PS->GetPlayerName() == It.PlayerName)
				{
					UE_LOG(LogTemp, Log, TEXT("플레이어 스테이트 네임 : %s"), *PS->GetPlayerName());
					UE_LOG(LogTemp, Log, TEXT("게임 스테이트에 저장된 네임 : %s"), *It.PlayerName);
                    
					ConnectedPlayerStates.Add(PS);
					NumPlayers++;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("PlayerState is empty."));
			}
		}
	}

	if (NumPlayers == MatchPlayersInfo.Num())
	{
		return true;
	}
	return false;
}

void APlayGameState::RemovePlayer(AGamePlayerController* WPlayerController)
{
	if (WPlayerController)
	{
		PlayerControllers.Remove(WPlayerController);
		ConnectedPlayerStates.Remove(WPlayerController->GetPlayerState<AGamePlayerState>());
		UE_LOG(LogTemp,Warning,TEXT("Player Removed"));
	}
}

void APlayGameState::CheckPlayerSpawned(AGamePlayerController* WPlayerController)
{
	UE_LOG(LogTemp, Log, TEXT("CheckPlayerSpawned %s"),*WPlayerController->GetName());
	CheckSpawnedPlayers++;
    
	if (CheckSpawnedPlayers == MatchPlayersInfo.Num())
	{
		SetGamePlay(E_GamePlay::ReadyCountdown);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("All Players are not Spawned"));
	}
}

void APlayGameState::CheckAllPlayersReady()
{
	bool bAllReady = true;

	for (APlayerState* PS : PlayerArray) // 모든 플레이어 상태 확인
	{
		AGamePlayerState* WPS = Cast<AGamePlayerState>(PS);
		if (WPS && !WPS->bIsGameReady) // 아직 준비 안된 플레이어가 있다면
		{
			bAllReady = false;
			break;
		}
	}

	if (bAllReady)
	{
		SetGamePlayerReady();
	}
}

void APlayGameState::CountInGameTime()
{
	InGameTime++;

	Multicast_CountInGameTime(InGameTime);

	// 미니언 소환
	if (InGameTime == 15.f)
	{
		PlayGameMode->SpawnMinions();
	}
}

void APlayGameState::Multicast_CountInGameTime_Implementation(float Time)
{
	InGameTime = Time;
}

void APlayGameState::ServerCountdown()
{
	PlayGameMode->SetGSPlayerControllers();
	PlayGameMode->StartCountdown(11);
}

void APlayGameState::SetCountdownTime(int32 NewCount)
{
	CountdownTime = NewCount;
	for (auto It : PlayerControllers)
	{
		It->CountdownTime = NewCount;
	}
}

void APlayGameState::SetGameStart()
{
	for (auto& It : PlayerControllers)
	{
		It->OnGameStateChanged(E_GamePlay::Gameplaying);
	}
}

void APlayGameState::AddRespawnTime()
{
	RespawnTime += 5;
}

void APlayGameState::AssignNexus(AAOSActor* SpawnedActor)
{
	if (ANexus* WNexus = Cast<ANexus>(SpawnedActor))
	{
		if (WNexus->TeamID == E_TeamID::Red)
		{
			RedNexus = WNexus;
		}
		else if (WNexus->TeamID == E_TeamID::Blue)
		{
			BlueNexus = WNexus;
		}
	}
}

float APlayGameState::GetBlueNexusHP()
{
	if (BlueNexus)
	{
		return BlueNexus->GetNexusHPPercent();
	}

	return 0;
}

float APlayGameState::GetRedNexusHP()
{
	if (RedNexus)   
	{
		return RedNexus->GetNexusHPPercent();
	}

	return 0;
}

void APlayGameState::OnRep_ManagedActors()
{
	CachedActors.Empty();
	for (AActor* Actor : GameManagedActors)
	{
		if (IsValid(Actor))
		{
			CachedActors.Add(Actor);
		}
	}
}

void APlayGameState::NM_ReplicateTotalKillPoints_Implementation(int32 Blue, int32 Red)
{
	BlueTeamTotalKillPoints = Blue;
	RedTeamTotalKillPoints = Red;

	DelegateShowKillState.Broadcast(BlueTeamTotalKillPoints, RedTeamTotalKillPoints);
}

void APlayGameState::CheckKilledTeam(E_TeamID KillTeam)
{
	if (KillTeam == E_TeamID::Blue)
	{
		BlueTeamTotalKillPoints++;
	}
	else if (KillTeam == E_TeamID::Red)
	{
		RedTeamTotalKillPoints++;
	}

	NM_ReplicateTotalKillPoints(BlueTeamTotalKillPoints, RedTeamTotalKillPoints);
}

void APlayGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GameManagedActors.Num() > 0)
	{
		for (auto& It : GameManagedActors)
		{
			if (!IsValid(It) || It->IsPendingKillPending())
				GameManagedActors.Remove(It);
		}
	}
}

void APlayGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (RespawnTimeHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnTimeHandle);
		GetWorld()->GetTimerManager().ClearTimer(InGameTimerHandle);
	}
}

void APlayGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APlayGameState, BlueTeamPlayerInfo);
	DOREPLIFETIME(APlayGameState, RedTeamPlayerInfo);
	DOREPLIFETIME(APlayGameState, BlueTeamPlayersNum);
	DOREPLIFETIME(APlayGameState, RedTeamPlayersNum);
	DOREPLIFETIME(APlayGameState, SelectCountdown);
	DOREPLIFETIME(APlayGameState, CurrentGamePhase);
	DOREPLIFETIME(APlayGameState, RedNexus);
	DOREPLIFETIME(APlayGameState, BlueNexus);
	DOREPLIFETIME(APlayGameState, GameManagedActors);
}
