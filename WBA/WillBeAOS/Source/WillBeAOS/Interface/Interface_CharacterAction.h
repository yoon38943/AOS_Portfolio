#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Interface_CharacterAction.generated.h"

UINTERFACE(MinimalAPI)
class UInterface_CharacterAction : public UInterface
{
	GENERATED_BODY()
};

class WILLBEAOS_API IInterface_CharacterAction
{
	GENERATED_BODY()

public:
	virtual void RequestSnapToCameraDirection(float SnapDirection) = 0;
};
