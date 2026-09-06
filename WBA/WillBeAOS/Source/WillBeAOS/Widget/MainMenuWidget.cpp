#include "MainMenuWidget.h"
#include "Widget/MainMenuWidget.h"
#include "LoginWaitingWidget.h"
#include "Components/Button.h"
#include "Game/WGameInstance.h"
#include "Components/WidgetSwitcher.h"
#include "Game/Network/WNetStatics.h"
#include "Kismet/KismetSystemLibrary.h"


void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RemoveLoadingScreen();

	if (!MainSwitcher) return;
	
	MainSwitcher->SetVisibility(ESlateVisibility::Collapsed);

	GameInstance = GetGameInstance<UWGameInstance>();
	if (GameInstance)
	{
		GameInstance->OnLoginCompleted.AddUObject(this, &ThisClass::LoginCompleted);

		GameInstance->AutoLogin();
		//GameInstance->ManualLogin();
		
		if (GameInstance->IsLoggedIn())
		{
			MainSwitcher->SetActiveWidgetIndex(1);
		}

		GameInstance->OnMatchmakingCompleted.AddUObject(this, &ThisClass::MatchmakingCompleted);
	}
	
	LoginButton->OnClicked.AddDynamic(this, &ThisClass::LoginBtnClicked);
	MatchingStartButton->OnClicked.AddDynamic(this, &ThisClass::MatchingStartButtonClicked);
	ExitGameButton->OnClicked.AddDynamic(this, &ThisClass::ExitGameBtnClicked);
}

void UMainMenuWidget::BackToLoginMenu()
{
	if (MainSwitcher)
	{
		MainSwitcher->SetActiveWidgetIndex(0);
	}
}

void UMainMenuWidget::SwitchToMainWidget()
{
	if (MainSwitcher)
	{
		MainSwitcher->SetActiveWidgetIndex(1);
	}
}

void UMainMenuWidget::ShowMainSwitcher()
{
	if (MainSwitcher)
	{
		MainSwitcher->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMainMenuWidget::MatchingStartButtonClicked()
{
	UE_LOG(LogTemp, Display, TEXT("Matching start"));
	if (GameInstance)
	{
		GameInstance->StartGlobalSessionSearch();
	}
	MatchingButtonClickedInBP();
}

void UMainMenuWidget::MatchmakingCompleted(bool bWasSuccessful)
{
	CompleteMatchingInBP(bWasSuccessful);

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("매칭 성공!!!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("매칭 실패..."));
	}
}

void UMainMenuWidget::LoginBtnClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("로그인 진행..."));
	if (GameInstance && !GameInstance->IsLoggedIn())
	{
		GameInstance->ManualLogin();
		SwitchToWaitingWidget(FText::FromString("Logging In..."));
	}
}

void UMainMenuWidget::ExitGameBtnClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UE_LOG(LogTemp, Log, TEXT("Cleaning up before quit..."));

	IOnlineSessionPtr SessionPtr = UWNetStatics::GetSessionPtr();
	if (SessionPtr)
	{
		SessionPtr->ClearOnDestroySessionCompleteDelegates(this);
		SessionPtr->ClearOnFindSessionsCompleteDelegates(this);
		SessionPtr->ClearOnJoinSessionCompleteDelegates(this);
	}
	
	UKismetSystemLibrary::QuitGame(World, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::RemoveLoadingScreen_Implementation()
{
	// 블루프린트에서 작성
}

void UMainMenuWidget::LoginCompleted(bool bWasSuccessful, const FString& UserId, const FString& Error)
{
	ShowMainSwitcher();
	
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("로그인 성공! UserId: %s"), *UserId);
		SwitchToMainWidget();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("로그인 실패! Error: %s"), *Error);
		BackToLoginMenu();
	}
}

void UMainMenuWidget::CompleteMatchingInBP_Implementation(bool bWasSuccessful)
{
}

void UMainMenuWidget::MatchingButtonClickedInBP_Implementation()
{
}

FOnButtonClickedEvent& UMainMenuWidget::SwitchToWaitingWidget(const FText& WaitInfo, bool bAllowcancel)
{
	MainSwitcher->SetActiveWidget(WaitingWidget);
	WaitingWidget->SetWaitInfo(WaitInfo, bAllowcancel);
	return WaitingWidget->ClearAndGetButtonClickedEvent();
}
