#include "Game/Network/WNetStatics.h"

#include "OnlineSubsystemUtils.h"

FOnlineSessionSettings UWNetStatics::GenerateOnlineSessionSettings(const FName& SessionName,
                                                                   const FString& SessionSearchId, int Port)
{
	FOnlineSessionSettings OnlineSessionSettings{};
	OnlineSessionSettings.bIsLANMatch = false;
	OnlineSessionSettings.NumPublicConnections = 1;
	OnlineSessionSettings.bShouldAdvertise = true;
	OnlineSessionSettings.bUsesPresence = false;
	OnlineSessionSettings.bAllowJoinViaPresence = false;
	OnlineSessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	OnlineSessionSettings.bAllowInvites = true;
	OnlineSessionSettings.bAllowJoinInProgress = true;
	OnlineSessionSettings.bUseLobbiesIfAvailable = false;
	OnlineSessionSettings.bUseLobbiesVoiceChatIfAvailable = false;
	OnlineSessionSettings.bUsesStats = true;
	
	return OnlineSessionSettings;
}

IOnlineSessionPtr UWNetStatics::GetSessionPtr()
{
	IOnlineSubsystem* OnlineSubSystem = IOnlineSubsystem::Get();
	if (OnlineSubSystem)
	{
		return OnlineSubSystem->GetSessionInterface();
	}
	return nullptr;
}

IOnlineIdentityPtr UWNetStatics::GetIdentityPtr(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;

	UWorld* World = WorldContext->GetWorld();
	if (World)
	{
		IOnlineSubsystem* OnlineSubSystem = Online::GetSubsystem(World);
		if (OnlineSubSystem)
		{
			return OnlineSubSystem->GetIdentityInterface();
		}
	}
	
	return nullptr;
}

bool UWNetStatics::IsSessionServer(const UObject* WorldContextObject)
{
	return WorldContextObject->GetWorld()->GetNetMode() == NM_DedicatedServer;
}

FName UWNetStatics::GetPortKey()
{
	return FName("GameLiftExternalPort");
}

FString UWNetStatics::GetCommandLineArgAsString(const FName& ParamName)
{
	FString OutVal = "";
	FString CommandLineArg = FString::Printf(TEXT("%s="), *(ParamName.ToString()));
	FParse::Value(FCommandLine::Get(), *CommandLineArg, OutVal);
	return OutVal;
}

int UWNetStatics::GetCommandLineArgAsInt(const FName& ParamName)
{
	int OutVal = 0;
	FString CommandLineArg = FString::Printf(TEXT("%s="), *(ParamName.ToString()));
	FParse::Value(FCommandLine::Get(), *CommandLineArg, OutVal);
	return OutVal;
}

void UWNetStatics::ReplacePort(FString& OutURLStr, int NewPort)
{
	FString IP;
	FString PortStr;
    
	// ":"를 기준으로 IP와 Port를 분리한 뒤 새로 조립
	if (OutURLStr.Split(TEXT(":"), &IP, &PortStr))
	{
		OutURLStr = FString::Printf(TEXT("%s:%d"), *IP, NewPort);
	}
}
