#include "PersistentGame/GamePlayerController.h"

#include "GamePlayerState.h"
#include "PlayGameMode.h"
#include "PlayGameState.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Character/WCharacterBase.h"
#include "Character/UI/SelectMapUserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "../Character/UI/TowerNexusHPWidget.h"
#include "../Character/WCharacterHUD.h"
#include "Game/WGameInstance.h"
#include "Net/UnrealNetwork.h"


void AGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("controller가 준비되었습니다."));

	if (IsLocalPlayerController() && !(GIsEditor && GWorld->IsPlayInEditor()))
	{
		LoadingWidget = CreateWidget<UUserWidget>(this, ToInGameLoadingWidgetClass);
		if (LoadingWidget)
		{
			LoadingWidget->AddToViewport(0);
		}
		
		CheckCharacterSelectLevelLoaded();
	}

	//에디터 테스트
	if (IsLocalController() && GetWorld() && GetWorld()->IsPlayInEditor())
	{
		PlayerHUD = CreateWidget<UWCharacterHUD>(this, UserWidgetClass);
		if (PlayerHUD)
		{
			PlayerHUD->AddToViewport();
		}
	}
}

void AGamePlayerController::CheckCharacterSelectLevelLoaded()
{
	FName LevelName = CharacterSelectLevel.ToSoftObjectPath().GetLongPackageFName();
	ULevelStreaming* LevelStream = UGameplayStatics::GetStreamingLevel(this, LevelName);

	if (LevelStream)
	{
		if (LevelStream->IsLevelVisible())
		{
			UE_LOG(LogTemp, Display, TEXT("레벨이 이미 로드되어 있습니다."));

			FTimerHandle RenderWaitTimer;
			GetWorld()->GetTimerManager().SetTimer(RenderWaitTimer, [this]()
			{
				Server_ControllerIsReady();
				Server_PossessControllerToCharacterSelect();
			}, 1.f, false);
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("레벨이 로드 중입니다. 로드 완료를 기다립니다..."));
			LevelStream->OnLevelShown.AddUniqueDynamic(this, &AGamePlayerController::OnLevelLoadedCallback);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("서브레벨 [%s]을 찾을 수 없습니다!"), *LevelName.ToString());
	}
}

void AGamePlayerController::OnLevelLoadedCallback()
{
	UE_LOG(LogTemp, Warning, TEXT("클라이언트 레벨 로드 완료!"));

	FTimerHandle RenderWaitTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderWaitTimer, [this]()
	{
		Server_ControllerIsReady();
		Server_PossessControllerToCharacterSelect();
	}, 0.5f, false);
}

void AGamePlayerController::Server_PossessControllerToCharacterSelect_Implementation()
{
	TArray<AActor*> FoundActors;
	AActor* PossessActor = UGameplayStatics::GetActorOfClass(this, PossessActorClass);

	if (!PossessActor)
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 선택 위치 액터를 찾을 수 없습니다!"));
		return;
	}
	
	FVector PossessLocation = PossessActor->GetActorLocation();
	FRotator PossessRotation = PossessActor->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	/*APawn* NewCameraPawn = GetWorld()->SpawnActor<APawn>(SpawnCameraClass, PossessLocation, PossessRotation, SpawnParams);
	if (NewCameraPawn)
	{
		// 4. 빙의 (서버에서 실행되므로 클라이언트로 자동 동기화됨)
		this->SetViewTarget(NewCameraPawn);
        
		UE_LOG(LogTemp, Warning, TEXT("캐릭터 선택 카메라 빙의 완료: %s"), *NewCameraPawn->GetName());
	}*/
}

void AGamePlayerController::StartCharacterSelectPhase()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AGamePlayerController::Server_ControllerIsReady_Implementation()
{
	APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
	if (GS)
	{
		GS->CheckPlayerIsReady(this);
	}
}

void AGamePlayerController::UpdatePlayerWidget_Implementation()
{
	USelectMapUserWidget* Widget = Cast<USelectMapUserWidget>(SelectCharacterWidget);
	if (Widget)
	{
		Widget->UpdateWidget();
	}
}

void AGamePlayerController::CloseLoadingScreen_Implementation()
{
	// 블루 프린트에서 StopLoadingScreen() 실행
}

void AGamePlayerController::Client_StartSelectCharacter_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("로딩창 지우기. 캐릭터 선택창으로 이동"));

	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
	}
	
	CloseLoadingScreen();
	
	SelectCharacterWidget = CreateWidget<UUserWidget>(this, SelectCharacterWidgetClass);
	if (SelectCharacterWidget)
	{
		SelectCharacterWidget->AddToViewport(0);
	}
}

void AGamePlayerController::BackToLobby_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("클라이언트 게임 로비로 복귀 중..."));

	if (MainLobbyLevel)
	{
		FString LevelName = MainLobbyLevel.ToSoftObjectPath().GetLongPackageName();
		
		ClientTravel(LevelName, TRAVEL_Absolute);
	}
}

void AGamePlayerController::ToInGameLoading_Implementation()
{
	if (SelectCharacterWidget && SelectCharacterWidget->IsInViewport())
	{
		SelectCharacterWidget->RemoveFromParent();
		SelectCharacterWidget = nullptr;
	}
	
	LoadingWidget = CreateWidget<UUserWidget>(this, ToInGameLoadingWidgetClass);
	if (LoadingWidget)
	{
		LoadingWidget->AddToViewport(0);
	}
}

void AGamePlayerController::CheckLoadedAllStreamingLevels()
{
	FName LastLevelName;
	
	APlayGameMode* GM = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		LastLevelName = FName(GM->StreamingLevelSequence.Last().GetAssetName());
	}

	ULevelStreaming* StreamingLevel = UGameplayStatics::GetStreamingLevel(this, LastLevelName);
	if (StreamingLevel && StreamingLevel->IsLevelLoaded() && StreamingLevel->IsLevelVisible())
	{
		// 레벨이 로드 됐으면 클라 준비 완료
		StartInGamePhase();		// 인게임 컨트롤러 실행
		
		AGamePlayerState* PS = GetPlayerState<AGamePlayerState>();
		if (PS)
		{
			PS->StartInGamePhase();		// 인게임 플레이어 스테이트 실행
		}
	}
	else
	{
		// 레벨이 클라에서 로드 되었는지 재확인
		FTimerHandle ReCheckTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(ReCheckTimerHandle, this, &ThisClass::CheckLoadedAllStreamingLevels, 0.1f, false);
	}
}

void AGamePlayerController::StartInGamePhase()
{
	// 인게임 전환 로딩창 비활성화
	if (LoadingWidget && LoadingWidget->IsInViewport())
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}

	// 다시 마우스 안보이도록
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	
	// 캐릭터 소환되면 인게임 위젯 붙이기
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);

	if (IsLocalController() && GameStateClass && UserWidgetClass)
	{
		// 월드 게임 플레이 관련
		if (!IsValid(GamePlayHUD))
		{
			GamePlayHUD = CreateWidget<UTowerNexusHPWidget>(this, GameStateClass);

			if (GamePlayHUD)
				GamePlayHUD->AddToViewport();
		}

		// 유저 위젯 관련
		if (!PlayerHUD)
		{
			PlayerHUD = CreateWidget<UWCharacterHUD>(this, UserWidgetClass);
			if (PlayerHUD)
			{
				PlayerHUD->SetAttributeSetStatInfo(GetPlayerState<AGamePlayerState>());
				PlayerHUD->AddToViewport();
			}
		}

		// 플레이어 준비 됐다고 신호 보내기
		Server_SetPlayerReady();
	}
}

void AGamePlayerController::BP_StartInGamePhase_Implementation()
{
	// 블프 내에서 구현
}

void AGamePlayerController::SetIsOpenStore_Implementation(bool CanOpen)
{
	IsOpenedStore = CanOpen;
}

void AGamePlayerController::S_SetCurrentRespawnTime_Implementation()
{
	if (APlayGameState* GameState = Cast<APlayGameState>(GetWorld()->GetGameState()))
	{
		CurrentRespawnTime = GameState->RespawnTime;
		C_ReplicateCurrentRespawnTime(CurrentRespawnTime);
	}
}

void AGamePlayerController::GameEnded_Implementation(E_TeamID LoseTeam)
{
	if (!IsLocalController()) return;
	
	if (GamePlayHUD)
		GamePlayHUD->RemoveFromParent();
	
	if(PlayerHUD)
		PlayerHUD->RemoveFromParent();

	AGamePlayerState* PS = GetPlayerState<AGamePlayerState>();
	if (PS)
	{
		PlayerTeamID = PS->InGamePlayerInfo.PlayerTeam;
	}
	
	if (PlayerTeamID != LoseTeam)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetWidgetToFocus(nullptr);

		SetInputMode(InputMode);
		SetShowMouseCursor(true);
		UUserWidget* WinScreen = CreateWidget(this, WinScreenClass);
		if (WinScreen != nullptr)
		{
			WinScreen->AddToViewport();
		}
	}
	else
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetWidgetToFocus(nullptr);

		SetInputMode(InputMode);
		SetShowMouseCursor(true);
		UUserWidget* LoseScreen = CreateWidget(this, LoseScreenClass);
		if (LoseScreen != nullptr)
		{
			LoseScreen->AddToViewport();
		}
	}
}

void AGamePlayerController::ShowRespawnWidget()
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (!RespawnScreen || !RespawnScreen->IsInViewport())
	{
		RespawnScreen = CreateWidget(GetWorld(), RespawnScreenClass);
		if (RespawnScreen)
		{
			RespawnScreen->AddToViewport(1); // ZOrder 조정
		}
	}
}

void AGamePlayerController::UpdateRespawnWidget()
{
	if (CurrentRespawnTime > 1)
	{
		CurrentRespawnTime--;
		C_ReplicateCurrentRespawnTime(CurrentRespawnTime);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
		HideRespawnWidget();
	}
}

void AGamePlayerController::HideRespawnWidget_Implementation()
{
	if (RespawnScreen != nullptr)
	{
		RespawnScreen->RemoveFromParent();
		RespawnScreen = nullptr;
	}
}

void AGamePlayerController::C_ReplicateCurrentRespawnTime_Implementation(int32 RespawnTime)
{
	CurrentRespawnTime = RespawnTime;
}

void AGamePlayerController::S_CountRespawnTime_Implementation()
{
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &ThisClass::UpdateRespawnWidget, 1.f, true);
}

void AGamePlayerController::PossessToSpectatorCamera(FVector CameraLocation, FRotator CameraRotation)
{
	FActorSpawnParameters SpawnParams;
	APawn* DeadCamera = GetWorld()->SpawnActor<APawn>(SpectorCamera, CameraLocation, CameraRotation, SpawnParams);

	if (DeadCamera)
	{
		Possess(DeadCamera);
	}
}

void AGamePlayerController::OnGameStateChanged(E_GamePlay CurrentGameState)
{
	switch (CurrentGameState)
	{
	case E_GamePlay::ReadyCountdown:
		DisableInput(this);
		break;
    
	case E_GamePlay::Gameplaying:
		EnableInput(this);
		break;
    
	case E_GamePlay::GameEnded:
		GameHasEnded();
		// 결과 화면 표시
		break;
    
	default:
		break;
	}
}

void AGamePlayerController::Server_SetPlayerReady_Implementation()
{
	AGamePlayerState* PS = GetPlayerState<AGamePlayerState>();
	if (PS)
	{
		PS->S_SetPlayerReady(true); // 플레이어 상태 변경
	}
}

void AGamePlayerController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);

	UE_LOG(LogTemp, Warning, TEXT("OnPossess"))
	
	AWCharacterBase* PlayerChar = Cast<AWCharacterBase>(NewPawn);
	if (PlayerChar)
	{
		if (AGamePlayerState* WPlayerState = GetPlayerState<AGamePlayerState>())
			PlayerChar->TeamID = WPlayerState->InGamePlayerInfo.PlayerTeam;
		UE_LOG(LogTemp, Log, TEXT("AWPlayerController::OnPossess %d"),PlayerChar->TeamID);
	}
}

void AGamePlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (PlayerHUD && GetWorld()->IsPlayInEditor())
	{
		PlayerHUD->SetAttributeSetStatInfo(GetPlayerState<AGamePlayerState>());
	}
}

void AGamePlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CountdownTime);
}
