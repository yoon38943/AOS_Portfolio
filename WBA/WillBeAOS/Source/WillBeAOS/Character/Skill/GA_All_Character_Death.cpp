#include "Character/Skill/GA_All_Character_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Camera/CameraComponent.h"
#include "Character/WCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/GamePlayerState.h"
#include "PersistentGame/PlayGameMode.h"
#include "PersistentGame/PlayGameState.h"

void UGA_All_Character_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                              const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PC = Cast<AGamePlayerController>(Avatar->GetController());
	PS = PC->GetPlayerState<AGamePlayerState>();

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* DeathMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Avatar->DeadAnimMontage, 1.f, NAME_None, false);
		DeathMontageTask->OnCompleted.AddDynamic(this, &ThisClass::SwitchToRagdoll);
		DeathMontageTask->ReadyForActivation();
	}

	if (K2_HasAuthority())
	{
		GameMode = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
		
		ApplyToAttacker();
		Character_Respawn_Server();
	}
	else
	{
		Character_Respawn_Client();
	}
}

void UGA_All_Character_Death::SwitchToRagdoll()
{
	Avatar->GetMesh()->bPauseAnims = true;
}

void UGA_All_Character_Death::ApplyToAttacker()
{
	if (Avatar->LastHitAttacker.IsValid())
	{
		Avatar->LastHitAttacker->AddKillPoint();
	}
	
	APlayGameMode* WGameMode = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (WGameMode)
	{
		WGameMode->OnObjectKilled(Avatar, Avatar->LastHitAttacker.Get());
	}
}

void UGA_All_Character_Death::Character_Respawn_Server()
{
	//캐릭터 리스폰
	APlayGameState* GameState = GameMode->GetGameState<APlayGameState>();
	if (Avatar && PC && PS && GameState && GameMode)
	{
		Avatar->GetCharacterMovement()->StopMovementImmediately();
		Avatar->GetCharacterMovement()->DisableMovement();
		Avatar->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Avatar->GetTeamIDCollision()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		Avatar->Dead_Multicast();
		
		PC->S_SetCurrentRespawnTime();
		PC->S_CountRespawnTime();

		PS->AddDeathPoint();
		
		GameState->GameManagedActors.Remove(Avatar);
		GameState->CheckKilledTeam(Avatar->GetTeamID());

		GameMode->SetResapwnPlayerTimer(Avatar, PC, GameState->RespawnTime);
		
		GetWorld()->GetTimerManager().SetTimer(SpawnSpectatorCameraTimerHandle, this, &ThisClass::PossessToSpectatorCamera, 2.5f, false);
	}
}

void UGA_All_Character_Death::Character_Respawn_Client()
{
	PC->SetIgnoreLookInput(true);
	PC->ShowRespawnWidget();
}

void UGA_All_Character_Death::PossessToSpectatorCamera()
{
	PC->PossessToSpectatorCamera(Avatar->GetFollowCamera()->GetComponentLocation(), Avatar->GetFollowCamera()->GetComponentRotation());
	GetWorld()->GetTimerManager().ClearTimer(SpawnSpectatorCameraTimerHandle);

	K2_EndAbility();
}
