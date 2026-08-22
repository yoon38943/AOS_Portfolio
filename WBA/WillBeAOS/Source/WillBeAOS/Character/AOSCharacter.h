#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/GetInfoInterface.h"
#include "AOSCharacter.generated.h"

UCLASS()
class WILLBEAOS_API AAOSCharacter : public ACharacter, public IGetInfoInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly)
	UCapsuleComponent* TeamTraceCollision;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	FName CharacterName;

public:
	AAOSCharacter();

	UCapsuleComponent* GetTeamIDCollision();

	UPROPERTY(BlueprintReadWrite)
	class ATower* TowerWithCharacterInside;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly)
	E_TeamID TeamID;

	virtual E_TeamID GetTeamID() const override { return TeamID; }
	virtual void SetTeamID(E_TeamID NewTeam) override { TeamID = NewTeam; }
	
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Gold")
	int32 GoldReward = 0;
	
	virtual int32 GetGoldReward() const {return GoldReward;};
	virtual void SetGoldReward(int32 NewGold){GoldReward = NewGold;}

	// 죽음 변수
	UPROPERTY(BlueprintReadWrite, Category = "Dead", Replicated)
	bool bIsDead = false;

	// 공격 록온 변수
	UPROPERTY(BlueprintReadWrite, Category = "LockOn")
	bool bIsEnemyLockOn;

	UPROPERTY(Replicated)
	bool IsRecalling;
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
};
