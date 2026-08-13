#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WBlueprintFunctionLibrary.generated.h"

struct FGameplayCueParameters;

UCLASS()
class WILLBEAOS_API UWBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "GameplayCue")
	static bool GetBulletContextData(const FGameplayCueParameters& CueParameters, FVector& MuzzleLoc, FVector& StrikePoint, float& Speed);
};
