#pragma once

#include "CoreMinimal.h"
#include "WEnumFile.h"
#include "UObject/Interface.h"
#include "GetInfoInterface.generated.h"

namespace CollisionInfo
{
	constexpr ECollisionChannel AOS_Pawn = ECC_GameTraceChannel1;
	constexpr ECollisionChannel BlueTeam = ECC_GameTraceChannel2;
	constexpr ECollisionChannel RedTeam = ECC_GameTraceChannel3;
	constexpr ECollisionChannel Perception = ECC_GameTraceChannel4;
}

UINTERFACE(MinimalAPI)

class UGetInfoInterface : public UInterface
{
	GENERATED_BODY()
};

class WILLBEAOS_API IGetInfoInterface
{
	GENERATED_BODY()

protected:
	

public:	//골드 관련
	virtual int32 GetGoldReward() const = 0;
	
public:	//팀 관련
	virtual E_TeamID GetTeamID() const = 0;
	virtual void SetTeamID(E_TeamID NewTeam) = 0;
};
