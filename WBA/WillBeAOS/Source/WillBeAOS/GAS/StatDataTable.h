#pragma once

#include "CoreMinimal.h"
#include "StatDataTable.generated.h"

USTRUCT(BlueprintType)
struct FStatDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health_Stat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Attack_Stat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Defense_Stat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed_Stat;
};
