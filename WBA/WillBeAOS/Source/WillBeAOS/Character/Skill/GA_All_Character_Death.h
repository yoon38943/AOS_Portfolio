#pragma once

#include "CoreMinimal.h"
#include "GAS/WGameplayAbility.h"
#include "GA_All_Character_Death.generated.h"

class AGamePlayerState;
class APlayGameMode;
class AGamePlayerController;

UCLASS()
class WILLBEAOS_API UGA_All_Character_Death : public UWGameplayAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

public:
	UPROPERTY()
	AGamePlayerController* PC;
	UPROPERTY()
	AGamePlayerState* PS;
	
	UPROPERTY()
	APlayGameMode* GameMode;

	FTimerHandle SpawnSpectatorCameraTimerHandle;

	UFUNCTION()
	void SwitchToRagdoll();

	void ApplyToAttacker();
	void Character_Respawn_Server();
	void Character_Respawn_Client();
	UFUNCTION()
	void PossessToSpectatorCamera();
};
