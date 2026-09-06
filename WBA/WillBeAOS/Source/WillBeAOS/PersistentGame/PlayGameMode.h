#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PlayGameMode.generated.h"

class UGameplayEffect;
class AWCharacterBase;
class IGetInfoInterface;
enum class E_TeamID : uint8;
class AGamePlayerState;
class APlayGameState;

UCLASS()
class WILLBEAOS_API APlayGameMode : public AGameMode
{
	GENERATED_BODY()

	APlayGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	UPROPERTY()
	APlayGameState* GS;

	virtual void BeginPlay() override;

	bool bIsGameEnded = false;
	
	// ---------------------------------------------
	// 플레이어 선택 게임 모드
	// ---------------------------------------------
public:
	UPROPERTY(EditAnywhere, Category="Level Streaming")
	TSoftObjectPtr<UWorld> CharacterSelectStream;

	UFUNCTION()
	void SelectLevelOnLoaded();
	
	void StartLoading();


	
	// ---------------------------------------------
	// 인게임 게임 모드
	// ---------------------------------------------
public:
	UPROPERTY(EditAnywhere, Category = "Level Streaming")
	TArray<TSoftObjectPtr<UWorld>> StreamingLevelSequence;

	int32 CurrentStreamingLevelIndex;

	void StartSequentialLevelStreaming();
	
	UFUNCTION()
	void OnSequenceLevelLoaded();

	void StartInGamePhase();

public:
	DECLARE_MULTICAST_DELEGATE(FOnGameEnd);
	FOnGameEnd OnGameEnd;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	APlayGameState* InGS;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	TSoftObjectPtr<UWorld> L_MainMenu;
	
public:	
	void SpawnTower();
	
	UPROPERTY(EditAnywhere)
	TArray<class AGamePlayerController*> PlayerControllers;
	
	void SetGSPlayerControllers();

	UPROPERTY()
	TArray<class APlayerSpawner*> PlayerSpawners;
	void GetPlayerSpawners();
	void SetPlayerSpawners(AGamePlayerState* PlayerState);

	void StartSpawnPlayers();

protected:
	int32 CountdownTime = 0;
	FTimerHandle CountdownHandle;
public:
	void StartCountdown(int32 InitialTime);
	void UpdateCountdown();

public:
	FTimerHandle SpawnMinionsTimerHandle;
	void SpawnMinions();
	
private:
	UPROPERTY()
	TMap<AActor*, int32> TeamMap;      // 팀정보 맵
public:
	// 팀 할당
	void AssignTeam(AActor* Actor, int32 TeamID);
	
	// 팀 검색
	int32 GetTeam(AActor* Actor) const;

protected:
	void SetArrayAllPlayerControllers();
	/*virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual bool ReadyToStartMatch_Implementation() override;*/
	virtual bool ReadyToEndMatch_Implementation() override;
	virtual void Logout(AController* Exiting) override;
	/*virtual void SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC) override;*/


public:
	// 스폰 관련
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;
	
	FTimerHandle SpawnTimerHandle;
	void SetResapwnPlayerTimer(AWCharacterBase* Player, AGamePlayerController* PlayerController, float RespawnTime);
	UFUNCTION()
	void RespawnPlayer(AWCharacterBase* Player, AGamePlayerController* PC);

	// 몬스터 사망시 이벤트
	UFUNCTION()
	void OnObjectKilled(TScriptInterface<IGetInfoInterface> DestroyedObject, AGamePlayerState* KillerPS);

	// 넥서스 파괴
	void OnNexusDestroyed(E_TeamID LoseTeam);
};
