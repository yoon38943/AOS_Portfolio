#include "PersistentGame/GamePlayerState.h"

#include "PlayGameMode.h"
#include "PlayGameState.h"
#include "Character/WCharacterBase.h"
#include "Character/WCharacterHUD.h"
#include "Character/Skill/SkillDataTable.h"
#include "Game/WGameInstance.h"
#include "GAS/WAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"


AGamePlayerState::AGamePlayerState()
{
	WAbilitySystemComponent = CreateDefaultSubobject<UWAbilitySystemComponent>(TEXT("ASC"));
	WAbilitySystemComponent->SetIsReplicated(true);
	WAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

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
		MaxHP = 200;
		SetHP(MaxHP);
		CPower = 20;
		CAdditionalHealth = 0;
		CDefense = 5;
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

void AGamePlayerState::C_SetSpeed_Implementation(float NewSpeed)
{
	ItemSpeed = NewSpeed;
	AWCharacterBase* Character = Cast<AWCharacterBase>(GetPawn());
	if (Character)
	{
		Character->UpdateMovementSpeedData(1.0f);
	}
}

void AGamePlayerState::AddSpeed_Implementation(float Speed)
{
	ItemSpeed += Speed;
	AWCharacterBase* Character = Cast<AWCharacterBase>(GetPawn());
	if (Character)
	{
		Character->UpdateMovementSpeedData(1.0f);
	}
	C_SetSpeed(ItemSpeed);
}

void AGamePlayerState::C_SetDefence_Implementation(float NewDefence)
{
	CDefense = NewDefence;
}

void AGamePlayerState::AddDefence_Implementation(float Defence)
{
	CDefense += Defence;
	C_SetDefence(CDefense);
}

void AGamePlayerState::C_SetHealth_Implementation(int32 NewHP, int32 NewMaxHP, int32 NewAddHealth)
{
	CAdditionalHealth = NewAddHealth;
    
	HP = NewHP;
	MaxHP = NewMaxHP;
}

void AGamePlayerState::AddHealth_Implementation(int32 Health)
{
	CAdditionalHealth += Health;

	HP += Health;
	MaxHP += Health;
	C_SetHealth(HP, MaxHP, CAdditionalHealth);
}

void AGamePlayerState::C_SetPower_Implementation(int32 NewPower)
{
	CPower = NewPower;
}

void AGamePlayerState::AddPower_Implementation(int32 Power)
{
	CPower += Power;
	C_SetPower(CPower);
}

void AGamePlayerState::Server_ApplyDamage_Implementation(int32 Damage, AController* AttackPlayer, AActor* DamageCauser)
{
	if (HP > 0)
	{
		const float Reduction = CDefense / (CDefense + 100.f);
		const int32 FinalDamage = StaticCast<int32>(Damage * (1.f - Reduction));
		if (HP > FinalDamage)
		{
			if (Cast<AWCharacterBase>(DamageCauser))
			{
				if (AttackPlayer && Cast<AWCharacterBase>(AttackPlayer->GetPawn()))
				{
					AGamePlayerState* AttackPS = AttackPlayer->GetPlayerState<AGamePlayerState>();
					AttackPS->PlayerDamageAmount += FinalDamage;
				}
			}

			HP -= FinalDamage;
		}
		else
		{
			if (Cast<AWCharacterBase>(DamageCauser))
			{
				if (AttackPlayer && Cast<AWCharacterBase>(AttackPlayer->GetPawn()))
				{
					AGamePlayerState* AttackPS = AttackPlayer->GetPlayerState<AGamePlayerState>();
					AttackPS->PlayerDamageAmount += HP;
				}
			}

			HP = 0;
		}
        
		if (HP <= 0)
		{
			AddDeathPoint(); // 데스 카운트 +1
			
			if (IsValid(AttackPlayer))
			{
				if (AWCharacterBase* AttackChar = Cast<AWCharacterBase>(AttackPlayer->GetPawn()))
				{
					AGamePlayerState* PS = AttackPlayer->GetPlayerState<AGamePlayerState>();
					PS->AddKillPoint();
					
					if (APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState()))
					{
						GS->CheckKilledTeam(PS->InGamePlayerInfo.PlayerTeam);
					}
				}                    
			}
            
			AWCharacterBase* PlayCharacter = Cast<AWCharacterBase>(GetPawn());
			if (PlayCharacter)
			{
				PlayCharacter->BeingDead();    // 캐릭터 사망처리
			}
		}
	}
}

bool AGamePlayerState::Server_ApplyDamage_Validate(int32 Damage, AController* AttackPlayer, AActor* DamageCauser)
{
	return Damage >= 0;
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
			GM->RespawnPlayer(nullptr, GetPlayerController());
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
