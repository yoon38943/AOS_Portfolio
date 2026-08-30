#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EWAbilityInputID : uint8
{
	None					UMETA(DisplayName = "None"),
	BasicAttack				UMETA(DisplayName = "BasicAttack"),
	Ability_RMB				UMETA(DisplayName = "Ability_RMB"),
	Ability_Q				UMETA(DisplayName = "Ability_Q"),
	Ability_E				UMETA(DisplayName = "Ability_E"),
	Ability_R				UMETA(DisplayName = "Ability_R"),
	Recall					UMETA(DisplayName = "Recall"),
	Confirm					UMETA(DisplayName = "Confirm"),
	Cancel					UMETA(DisplayName = "Cancel")
};