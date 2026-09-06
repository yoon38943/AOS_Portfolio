#include "PersistentGame/GamePlayerState.h"

#include "GamePlayerController.h"
#include "PlayGameMode.h"
#include "PlayGameState.h"
#include "Character/WCharacterBase.h"
#include "Character/WCharacterHUD.h"
#include "Character/Skill/SkillDataTable.h"
#include "Game/WGameInstance.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "Net/UnrealNetwork.h"


AGamePlayerState::AGamePlayerState()
{
	WAbilitySystemComponent = CreateDefaultSubobject<UWAbilitySystemComponent>(TEXT("ASC"));
	WAbilitySystemComponent->SetIsReplicated(true);
	WAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	WAttributeSet = CreateDefaultSubobject<UWAttributeSet>(TEXT("AttributeSet"));
	
	bReplicates = true;
	SetNetUpdateFrequency(100.f);
}

void AGamePlayerState::BeginPlay()
{
	Super::BeginPlay();

	// 에디터에서 테스트할 때 사용
	//StartInGamePhase();
}

void AGamePlayerState::StartCharacterSelectPhase()
{
	Server_ReplicatePlayerInfo(GetPlayerName());
}

void AGamePlayerState::Client_PlayerInfoReady_Implementation(FPlayerInfoStruct PlayerInfoStruct)
{
	UWGameInstance* GI = Cast<UWGameInstance>(GetGameInstance());
	if (GI)
	{
		PlayerInfo = PlayerInfoStruct;
		UE_LOG(LogTemp, Warning, TEXT("%s, %s"), *PlayerInfo.PlayerName, *PlayerInfo.PlayerNickName);
	}
}

void AGamePlayerState::StartInGamePhase()
{
	bReplicates = true;

	if (HasAuthority())
	{
		// Initialize default values for the player's stats
		CCurrentExp = 0;
		CExperience = 100;
		CLevel = 0;
		CAbLevel = 0;
		CGold = 0;

		ForceNetUpdate();
	}
	
	if (!HasAuthority())
	{
		Server_TakePlayerInfo(GetPlayerName());
	}
}

void AGamePlayerState::OnRep_AddSkillIcon()
{
	LoadSkillIcon.ExecuteIfBound();
}

void AGamePlayerState::OnRep_Health()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_Health called! HP: %f, MaxHP: %f"), HP, MaxHP);
	OnHealthChanged.Broadcast(GetHPPercentage());
}

float AGamePlayerState::GetHP()
{
	return HP;
}

float AGamePlayerState::GetMaxHP()
{
	return MaxHP;
}

void AGamePlayerState::SetHP(int32 NewHP)
{
	HP = NewHP;
}

float AGamePlayerState::GetHPPercentage()
{
	return static_cast<float>(HP) / static_cast<float>(MaxHP);
}

void AGamePlayerState::AddSpeed_Implementation(float Speed)
{
	FGameplayEffectContextHandle ContextHandle = WAbilitySystemComponent->MakeEffectContext();

	FGameplayEffectSpecHandle SpecHandle = WAbilitySystemComponent->MakeOutgoingSpec(AddSpeedEffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	WAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	
	AWCharacterBase* Character = Cast<AWCharacterBase>(GetPawn());
	if (Character)
	{
		Character->UpdateMovementSpeedData(1.0f);
	}
}

void AGamePlayerState::AddDefence_Implementation(float Defence)
{
	FGameplayEffectContextHandle ContextHandle = WAbilitySystemComponent->MakeEffectContext();

	FGameplayEffectSpecHandle SpecHandle = WAbilitySystemComponent->MakeOutgoingSpec(AddDefenseEffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	WAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void AGamePlayerState::AddHealth_Implementation(int32 Health)
{
	FGameplayEffectContextHandle ContextHandle = WAbilitySystemComponent->MakeEffectContext();

	FGameplayEffectSpecHandle SpecHandle = WAbilitySystemComponent->MakeOutgoingSpec(AddHealthEffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	WAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void AGamePlayerState::AddPower_Implementation(int32 Power)
{
	FGameplayEffectContextHandle ContextHandle = WAbilitySystemComponent->MakeEffectContext();

	FGameplayEffectSpecHandle SpecHandle = WAbilitySystemComponent->MakeOutgoingSpec(AddAttackEffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	WAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void AGamePlayerState::Server_TakePlayerInfo_Implementation(const FString& PlayerName)
{
	// 게임 스테이트에서 플레이어 정보 받아오기
	APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
	if (GS)
	{
		for (auto& Elem : GS->MatchPlayersInfo)
		{
			if (Elem.PlayerName == PlayerName)
			{
				InGamePlayerInfo = Elem;
				UE_LOG(LogTemp, Log, TEXT("플레이어(%s) 정보 받아오기 완료!"), *PlayerName);
			}
		}

		// 정보 받아와서 캐릭터 소환
		APlayGameMode* GM = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			AGamePlayerController* PC = Cast<AGamePlayerController>(GetPlayerController());
			GM->RespawnPlayer(nullptr, PC);
		}
	}
}

void AGamePlayerState::S_SetPlayerReady_Implementation(bool bReady)
{
	bIsGameReady = bReady;

	// 서버의 모든 플레이어가 준비가 되어있는지 확인
	APlayGameState* GS = GetWorld()->GetGameState<APlayGameState>();
	if (GS)
	{
		GS->CheckAllPlayersReady();
	}
}

bool AGamePlayerState::S_SetPlayerReady_Validate(bool bReady)
{
	return true;
}

void AGamePlayerState::Server_ReplicatePlayerInfo_Implementation(const FString& ClientPlayerName)
{
	UE_LOG(LogTemp, Log, TEXT("플레이어 네임 : %s"), *ClientPlayerName);
	
	PlayerInfo.PlayerName = *ClientPlayerName;

	UWGameInstance* GI = Cast<UWGameInstance>(GetGameInstance());
	if (GI)
	{
		UE_LOG(LogTemp, Log, TEXT("플레이어 네임 : %s - 맵 이동 완료(Select Character Map)"), *PlayerInfo.PlayerName);
		
		if (GI->MatchPlayersTeamInfo.Contains(PlayerInfo.PlayerName))
		{
			PlayerInfo = GI->GetSavedPlayerTeamInfo()[PlayerInfo.PlayerName];
			UE_LOG(LogTemp, Log, TEXT("플레이어 팀 정보 : %s"), PlayerInfo.PlayerTeam == E_TeamID::Blue? TEXT("블루팀") : TEXT("레드팀"));
		}
	}

	Client_PlayerInfoReady(PlayerInfo);
}

void AGamePlayerState::Server_ChooseTheCharacter_Implementation(TSubclassOf<APawn> ChosenChar, FName CharacterName)
{
	InGamePlayerInfo.SelectedCharacter = ChosenChar;
	APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
	{
		GS->AddSelectCharacterToPlayerInfo(GetPlayerName(), ChosenChar, PlayerInfo.PlayerTeam, CharacterName);
	}
}

void AGamePlayerState::Server_AddGold_Implementation(int Amount)
{
	CGold += Amount;
	C_AddGold(CGold);
}

void AGamePlayerState::C_AddGold_Implementation(int NewGold)
{
	CGold = NewGold;
}

void AGamePlayerState::AddDeathPoint_Implementation()
{
	PlayerDeathCount++;
}

void AGamePlayerState::AddKillPoint_Implementation()
{
	PlayerKillCount++;
}

void AGamePlayerState::Server_RequestUseSkill_Implementation(ESkillSlot SkillSlot)
{
	if (IsSkillReady(SkillSlot))
	{
		StartCooldown(SkillSlot);

		AWCharacterBase* PlayChar = GetPawn<AWCharacterBase>();
		if (PlayChar)
		{
			PlayChar->ExecuteSkill(SkillSlot);
		}
	}
}

void AGamePlayerState::Client_Delegate_UsedSkill_Implementation(FSkillUsedInfo LastUsedSkill)
{
	OnSkillCooldown.Broadcast(LastUsedSkill);
}

bool AGamePlayerState::IsSkillReady(ESkillSlot Skillslot)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();

	for (FSkillUsedInfo& CooldownData : SkillCooldownEndTimes)
	{
		if (CooldownData.SkillSlot == Skillslot)
		{
			if (CurrentTime < CooldownData.CooldownTimeEnd)
			{
				return false;
			}
		}
	}

	return true;
}

void AGamePlayerState::StartCooldown(ESkillSlot Skillslot)
{
	AWCharacterBase* PlayChar = GetPawn<AWCharacterBase>();
	if (PlayChar)
	{
		FName SkillID;
		switch (Skillslot)
		{
		case ESkillSlot::Q:
			SkillID = "QSkill";
			break;
		case ESkillSlot::E:
			SkillID = "ESkill";
			break;
		case ESkillSlot::R:
			SkillID = "RSkill";
			break;
		}
		
		FSkillDataTable* SkillInfo = PlayChar->SkillDataTable->FindRow<FSkillDataTable>(SkillID, TEXT(""));
		if (SkillInfo)
		{
			float EndCooldownTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + SkillInfo->SkillCooldownTime;

			bool bFound = false;
			for (FSkillUsedInfo& CooldownData : SkillCooldownEndTimes)
			{
				if (CooldownData.SkillSlot == Skillslot)
				{
					CooldownData.CooldownTimeEnd = EndCooldownTime;
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				FSkillUsedInfo NewData;
				NewData.SkillSlot = Skillslot;
				NewData.CooldownTimeEnd = EndCooldownTime;
				SkillCooldownEndTimes.Add(NewData);
			}

			LastSkillCooldown.SkillSlot = Skillslot;
			LastSkillCooldown.CooldownTime = SkillInfo->SkillCooldownTime;
			LastSkillCooldown.CooldownTimeEnd = EndCooldownTime;
			Client_Delegate_UsedSkill(LastSkillCooldown);
		}
	}
}

int32 AGamePlayerState::GetKillPoints()
{
	return PlayerKillCount;
}

int32 AGamePlayerState::GetDeathPoints()
{
	return PlayerDeathCount;
}

void AGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGamePlayerState, PlayerInfo);
	DOREPLIFETIME(AGamePlayerState, InGamePlayerInfo);
	DOREPLIFETIME(AGamePlayerState, HP);
	DOREPLIFETIME(AGamePlayerState, MaxHP);
	DOREPLIFETIME(AGamePlayerState, SkillCooldownEndTimes);
}
