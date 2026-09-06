#include "PersistentGame/PlayGameMode.h"

#if defined(WITH_GAMELIFT) && WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#endif
#include "GamePlayerController.h"
#include "GamePlayerState.h"
#include "PlayGameState.h"
#include "Character/AOSActor.h"
#include "Character/AOSCharacter.h"
#include "Character/WCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Game/Network/WGameSession.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/WAbilitySystemComponent.h"
#include "Gimmick/PlayerSpawner.h"
#include "Gimmick/SpawnTowerPoint.h"
#include "Gimmick/Tower.h"
#include "Kismet/GameplayStatics.h"
#include "Minions/MinionsSpawner.h"


APlayGameMode::APlayGameMode()
{
	GameSessionClass = AWGameSession::StaticClass();
}

void APlayGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// 이미 다른 이유로 접속이 거부되었다면 통과
	if (!ErrorMessage.IsEmpty()) return;
	
	if (bIsGameEnded)
	{
		ErrorMessage = TEXT("이미 종료된 게임 세션입니다.");
		UE_LOG(LogTemp, Warning, TEXT("종료된 게임에 난입 시도 발생 -> 접속 거부 처리 완료"));
	}
}

void APlayGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!CharacterSelectStream.IsNull())
	{
		FLatentActionInfo LoadSelectCharLevelLatentInfo;
		LoadSelectCharLevelLatentInfo.CallbackTarget = this;
		LoadSelectCharLevelLatentInfo.ExecutionFunction = FName("SelectLevelOnLoaded");
		LoadSelectCharLevelLatentInfo.Linkage = 0;
		LoadSelectCharLevelLatentInfo.UUID = __LINE__;

		FName SelectCharLevelName = FName(*CharacterSelectStream.GetAssetName());
		UGameplayStatics::LoadStreamLevel(this, SelectCharLevelName, true, false, LoadSelectCharLevelLatentInfo);
	}
}

void APlayGameMode::SelectLevelOnLoaded()
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *this->GetName());
	
	GS = GetWorld()->GetGameState<APlayGameState>();
	if (GS)
	{
		GS->SetGamePhase(EGamePhase::CharacterSelect);
	}
}

void APlayGameMode::StartLoading()
{
	if (GS)
	{
		GS->SetGamePhase(EGamePhase::LoadingPhase);
	}
	
	FLatentActionInfo UnloadSelectCharLevelLatentInfo;
	UnloadSelectCharLevelLatentInfo.CallbackTarget = this;
	UnloadSelectCharLevelLatentInfo.Linkage = 0;
	UnloadSelectCharLevelLatentInfo.UUID = __LINE__;

	FName SelectCharLevelName = FName(*CharacterSelectStream.GetAssetName());
	UGameplayStatics::UnloadStreamLevel(this, SelectCharLevelName,UnloadSelectCharLevelLatentInfo, false);

	StartSequentialLevelStreaming();
}

void APlayGameMode::StartSequentialLevelStreaming()
{
	if (StreamingLevelSequence.IsValidIndex(CurrentStreamingLevelIndex))
	{
		FName LevelName = FName(*StreamingLevelSequence[CurrentStreamingLevelIndex].GetAssetName());

		FLatentActionInfo SequenceLatentInfo;
		SequenceLatentInfo.CallbackTarget = this;
		SequenceLatentInfo.ExecutionFunction = FName("OnSequenceLevelLoaded");
		SequenceLatentInfo.Linkage = 0;
		SequenceLatentInfo.UUID = __LINE__;

		UGameplayStatics::LoadStreamLevel(this, LevelName, true, false, SequenceLatentInfo);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("모든 스트리밍 레벨 로드 완료!"));

		StartInGamePhase();
		if (GS)
		{
			GS->SetGamePhase(EGamePhase::InGame);
		}
	}
}

void APlayGameMode::OnSequenceLevelLoaded()
{
	UE_LOG(LogTemp, Log, TEXT("%s 로드 완료"), *StreamingLevelSequence[CurrentStreamingLevelIndex]->GetName());

	AGamePlayerController* PC = Cast<AGamePlayerController>(GetWorld()->GetFirstPlayerController());

	CurrentStreamingLevelIndex++;
	StartSequentialLevelStreaming(); // 다음 레벨 로드
}

void APlayGameMode::StartInGamePhase()
{
	UE_LOG(LogTemp, Log, TEXT("InGame GameMode BeginPlay called"));
	
	InGS = GetWorld()->GetGameState<APlayGameState>();
	if (!InGS)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameState is nullptr!"));
	}

	SetArrayAllPlayerControllers();
	SpawnTower();
	GetPlayerSpawners();
}

void APlayGameMode::SpawnTower()
{
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnTowerPoint::StaticClass(), SpawnPoints);

	for (AActor* SpawnPointActor : SpawnPoints)
	{
		ASpawnTowerPoint* SpawnPoint = Cast<ASpawnTowerPoint>(SpawnPointActor);
		
		if (HasAuthority() && SpawnPoint && SpawnPoint->TowerClass)
		{
			// 타워 생성
			FTransform SpawnTransform = FTransform(SpawnPoint->GetActorRotation(), SpawnPoint->GetActorLocation());
			AAOSActor* Tower = GetWorld()->SpawnActorDeferred<AAOSActor>(
				SpawnPoint->TowerClass,
				SpawnTransform,
				SpawnPoint
			);
			
			if (Tower)
			{
				Tower->SetReplicates(true);
				Tower->SetTeamID(SpawnPoint->TeamID);
				InGS->AssignNexus(Tower);
				AssignTeam(Tower,static_cast<int32>(Tower->TeamID));
				ATower* TowerColor = Cast<ATower>(Tower);
				if (TowerColor)
				{
					TowerColor->S_SetHPbarColor();
				}

				UGameplayStatics::FinishSpawningActor(Tower, SpawnPoint->GetActorTransform());
			}
		}
	}
}

void APlayGameMode::SetGSPlayerControllers()
{
	InGS->PlayerControllers = PlayerControllers;
}

void APlayGameMode::GetPlayerSpawners()
{
	TArray<AActor*> PlayerSpawnPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),APlayerSpawner::StaticClass(), PlayerSpawnPoints);

	for (auto It:PlayerSpawnPoints)
	{
		if (APlayerSpawner* Spawner = Cast<APlayerSpawner>(It))
		{
			PlayerSpawners.Add(Spawner);
		}
	}
	UE_LOG(LogTemp,Log,TEXT("Player Spawners Num %d "),PlayerSpawners.Num());
}

void APlayGameMode::SetPlayerSpawners(AGamePlayerState* PlayerState)
{
	for (auto It: PlayerSpawners)
	{
		if (It->TeamID == PlayerState->InGamePlayerInfo.PlayerTeam)		
		{
			PlayerState->PlayerSpawner = It;
		}
	}
}

void APlayGameMode::StartSpawnPlayers()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC)
		{
			AGamePlayerController* PlayerController = Cast<AGamePlayerController>(PC);
			if (PlayerController)
			{
				RespawnPlayer(nullptr, PlayerController);
			}
		}
	}
}

void APlayGameMode::StartCountdown(int32 InitialTime)
{
	UE_LOG(LogTemp, Log, TEXT("ServerCountdown"));
	CountdownTime = InitialTime;
	GetWorldTimerManager().SetTimer(CountdownHandle, this, &APlayGameMode::UpdateCountdown, 1.f, true);
}

void APlayGameMode::UpdateCountdown()
{
	CountdownTime--;
	
	if (CountdownTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(CountdownHandle);
		InGS->SetGamePlay(E_GamePlay::Gameplaying);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("UpdateCountdown %d"),CountdownTime);
		InGS->SetCountdownTime(CountdownTime);	
	}
}

void APlayGameMode::SpawnMinions()
{
	TArray<AActor*> MinionSpawnPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),AMinionsSpawner::StaticClass(), MinionSpawnPoints);
	if (MinionSpawnPoints.Num() > 0)
	{
		for (auto It:MinionSpawnPoints)
		{
			if (AMinionsSpawner* MinionsSpawner = Cast<AMinionsSpawner>(It))
			{
				MinionsSpawner->StartSpawnMinions();
			}
		}
	}
}

void APlayGameMode::AssignTeam(AActor* Actor, int32 TeamID)
{
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid actor!"));
		return;
	}
	TeamMap.Add(Actor, TeamID);
	UE_LOG(LogTemp, Log, TEXT("Actor Add! %s %d"), *Actor->GetName(), TeamID);
}

int32 APlayGameMode::GetTeam(AActor* Actor) const
{
	if (!Actor)
	{
		return -1; // Invalid actor
	}
	
	if (const int32* TeamID = TeamMap.Find(Actor))
	{
		return *TeamID;
	}

	return -1; // Team 정보 없음
}

void APlayGameMode::SetArrayAllPlayerControllers()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC)
		{
			AGamePlayerController* GPC = Cast<AGamePlayerController>(PC);
			if (GPC)
			{
				PlayerControllers.AddUnique(GPC);
				GPC->CheckLoadedAllStreamingLevels();
				if (InGS)
				{
					SetGSPlayerControllers();
				}
			}
		}
	}
}

bool APlayGameMode::ReadyToEndMatch_Implementation()
{
	Super::ReadyToEndMatch_Implementation();

	return (InGS != nullptr) && (InGS->CurrentInGamePhase == E_GamePlay::GameEnded);
}

void APlayGameMode::Logout(AController* Exiting)
{
	if (InGS)
	{
		if (InGS->CurrentInGamePhase == E_GamePlay::Gameplaying)
		{
			AGamePlayerController* GamePlayerController = Cast<AGamePlayerController>(Exiting);
			if (GamePlayerController)
			{
				InGS->RemovePlayer(GamePlayerController);
			}
		}
	}

	Super::Logout(Exiting);
}

void APlayGameMode::SetResapwnPlayerTimer(AWCharacterBase* Player, AGamePlayerController* PlayerController, float RespawnTime)
{
	FTimerDelegate SpawnTimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::RespawnPlayer, Player, PlayerController);
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, SpawnTimerDelegate, RespawnTime, false);
}

void APlayGameMode::RespawnPlayer(AWCharacterBase* Player, AGamePlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	
	AGamePlayerState* PS = PC->GetPlayerState<AGamePlayerState>();
	if (PS)
	{
		if (Player == nullptr)
		{
			SetPlayerSpawners(PS);

			if (!PS->PlayerSpawner)
			{
				UE_LOG(LogTemp, Warning, TEXT("Player Spawner NULL"));
				return;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			
			AWCharacterBase* RespawnChar = GetWorld()->SpawnActor<AWCharacterBase>(PS->InGamePlayerInfo.SelectedCharacter, PS->PlayerSpawner->GetActorLocation(), PS->PlayerSpawner->GetActorRotation(), Params);
			if (RespawnChar)
			{
				UE_LOG(LogTemp, Log, TEXT("Player Spawn : %s, %d"),*PC->GetName(),PS->InGamePlayerInfo.PlayerTeam);

				RespawnChar->TeamID = PS->InGamePlayerInfo.PlayerTeam;
				
				RespawnChar->SetActorRotation(PS->PlayerSpawner->GetActorRotation());
				
				PC->OnPossess(RespawnChar);
				PC->SetControlRotation(PS->PlayerSpawner->GetActorRotation());
				PC->OnGameStateChanged(E_GamePlay::ReadyCountdown);
				InGS->CheckPlayerSpawned(PC);
				UE_LOG(LogTemp, Log, TEXT("첫 스폰!"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed Spawn Player"));
			}
		}
		else
		{
			if (PS->PlayerSpawner)
			{
				Player->Respawn_Multicast(PS->PlayerSpawner->GetActorLocation(), PS->PlayerSpawner->GetActorRotation());
			}
			else
			{
				Player->Respawn_Multicast(FVector(21490.f,9960.f,150.f), FRotator(0,0,0));
			}
			
			PC->GetPawn()->Destroyed();
			PC->OnPossess(Player);
			Player->Respawn_Client();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("리스폰 %s PlayerState 없음!"),*PC->GetName());
	}
}

void APlayGameMode::OnObjectKilled(TScriptInterface<IGetInfoInterface> DestroyedObject, AGamePlayerState* KillerPS)
{
	if (!DestroyedObject || !KillerPS) return;
	
	KillerPS->Server_AddGold(DestroyedObject->GetGoldReward());
}

void APlayGameMode::OnNexusDestroyed(E_TeamID LoseTeam)
{
	if (!HasAuthority()) return;

	bIsGameEnded = true;

#if defined(WITH_GAMELIFT) && WITH_GAMELIFT
	// GameLift에게 더 이상 새로운 플레이어 세션을 만들지 못하게 막음.
	FGameLiftServerSDKModule* GameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));
	GameLiftSdkModule->UpdatePlayerSessionCreationPolicy(EPlayerSessionCreationPolicy::DENY_ALL);
#endif

	APlayGameState* FGS = GetGameState<APlayGameState>();
	if (FGS)
	{
		FGS->SetGamePlay(E_GamePlay::GameEnded);
	}
	//고쳐야함

	OnGameEnd.Broadcast();
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGamePlayerController* PC = Cast<AGamePlayerController>(It->Get());
		if (PC)
		{
			PC->GameEnded(LoseTeam);
		}
	}
}
