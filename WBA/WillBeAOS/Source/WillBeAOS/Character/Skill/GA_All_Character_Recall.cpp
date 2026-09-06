#include "Character/Skill/GA_All_Character_Recall.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Blueprint/UserWidget.h"
#include "Character/WCharacterBase.h"
#include "Character/WCharAnimInstance.h"
#include "GameFramework/PlayerStart.h"
#include "Gimmick/PlayerSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/GamePlayerState.h"
#include "Widget/RecallWidget.h"


void UGA_All_Character_Recall::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                               const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (RecallEffectClass)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		ASC->ApplyGameplayEffectToSelf(RecallEffectClass.GetDefaultObject(), 1.0f, Context);
	}
	
	if (Avatar)
	{
		RecallMontage = Avatar->GetStartRecallMontage();
		CompleteRecallMontage = Avatar->GetCompleteRecallMontage();
		RecallCueTag = Avatar->GetRecallCueTag();
	}
	
	Avatar->IsRecalling = true;
	
	Avatar->MultiPlayMontage(RecallMontage);

	GetWorld()->GetTimerManager().SetTimer(RecallTimerHandle, this, &ThisClass::CompleteRecall, RecallTime, false);

	Avatar->StartRecall(RecallWidgetClass, RecallTime);

	if (ASC && HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
		ASC->AddGameplayCue(RecallCueTag);
}

void UGA_All_Character_Recall::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_All_Character_Recall::CompleteRecall()
{
	Avatar->IsRecalling = false;
	
	RecallToBase();

	Avatar->EndRecall();
		
	if (Avatar && CompleteRecallMontage)
	{
		Avatar->MultiPlayMontage(CompleteRecallMontage);
	}

	K2_EndAbility();
}

void UGA_All_Character_Recall::RecallToBase()
{
	if (GetWorld()->IsPlayInEditor())
	{
		TArray<AActor*> FoundPlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundPlayerStarts);

		for (AActor* Actor : FoundPlayerStarts)
		{
			APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
			if (PlayerStart)
			{
				Avatar->TeleportTo(PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation());
				Avatar->MultiClientSetRotation(PlayerStart->GetActorRotation());
				break;
			}
		}
	}
	else
	{
		AGamePlayerController* Controller = Cast<AGamePlayerController>(Avatar->GetController());
		if (!Controller) return;
		
		AGamePlayerState* WPlayerState = Cast<AGamePlayerState>(Controller->PlayerState);
		if (!WPlayerState) return;
	
		if (Avatar && WPlayerState->PlayerSpawner)
		{
			FRotator LookCenter = (FVector(0, 0, 100) - Avatar->GetActorLocation()).Rotation();
			Avatar->TeleportTo(WPlayerState->PlayerSpawner->GetActorLocation(), LookCenter);
			Avatar->MultiClientSetRotation(LookCenter);
		}
	}
}

void UGA_All_Character_Recall::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                          bool bReplicateEndAbility, bool bWasCancelled)
{
	Avatar->EndRecall();
	Avatar->IsRecalling = false;

	if (ASC && HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
		ASC->RemoveGameplayCue(RecallCueTag);
	
	if (RecallTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(RecallTimerHandle);

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("ability.state.recall")));
	ASC->RemoveActiveEffectsWithGrantedTags(TagContainer);
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}