#include "Character/Skill/GA_All_Character_Recall.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Blueprint/UserWidget.h"
#include "Character/WCharacterBase.h"
#include "Gimmick/PlayerSpawner.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/GamePlayerState.h"
#include "Widget/RecallWidget.h"


void UGA_All_Character_Recall::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                               const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (Avatar)
	{
		RecallMontage = Avatar->GetStartRecallMontage();
		CompleteRecallMontage = Avatar->GetCompleteRecallMontage();
		RecallCueTag = Avatar->GetRecallCueTag();
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		Avatar->IsRecalling = true;
		
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, RecallMontage);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
		MontageTask->ReadyForActivation();

		GetWorld()->GetTimerManager().SetTimer(RecallTimerHandle, this, &ThisClass::CompleteRecall, RecallTime, false);
	}

	if (!K2_HasAuthority())
	{
		CallRecall_Client();
	}

	if (ASC && HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
		ASC->AddGameplayCue(RecallCueTag);

	UAbilityTask_WaitInputPress* WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputTask->OnPress.AddDynamic(this, &ThisClass::OnRecallPressAgain);
	WaitInputTask->ReadyForActivation();
}

void UGA_All_Character_Recall::CallRecall_Client()
{
	if (IsLocallyControlled())
	{
		ShowRecallWidget();
	}
}

void UGA_All_Character_Recall::CompleteRecall()
{
	Avatar->IsRecalling = false;

	if (K2_HasAuthority())
	{
		RecallToBase();
	}
	else
	{
		if (IsLocallyControlled())
		{
			HideRecallWidget();
		}
		
		if (Avatar && CompleteRecallMontage)
		{
			Avatar->PlayAnimMontage(CompleteRecallMontage);
		}
	}

	K2_EndAbility();
}

void UGA_All_Character_Recall::RecallToBase()
{
	AGamePlayerController* Controller = Cast<AGamePlayerController>(Avatar->GetController());
	if (!Controller) return;

	AGamePlayerState* WPlayerState = Cast<AGamePlayerState>(Controller->PlayerState);
	if (!WPlayerState) return;
	
	if (Avatar && WPlayerState->PlayerSpawner)
	{
		Avatar->SetActorLocation(WPlayerState->PlayerSpawner->GetActorLocation());
		FRotator LookCenter = (FVector(0, 0, 100) - Avatar->GetActorLocation()).Rotation();
		Avatar->SetActorRotation(LookCenter);
		Avatar->GetController()->SetControlRotation(LookCenter);
	}
}

void UGA_All_Character_Recall::ShowRecallWidget()
{
	if (RecallWidgetClass && !RecallWidget)
	{
		RecallWidget = CreateWidget<URecallWidget>(Cast<AGamePlayerController>(Avatar->GetController()), RecallWidgetClass);
		if (RecallWidget)
		{
			RecallWidget->RecallTime = RecallTime;
			RecallWidget->StartRecalling();
			RecallWidget->AddToViewport();
		}
	}
}

void UGA_All_Character_Recall::HideRecallWidget()
{
	if (RecallWidget && RecallWidget->IsInViewport())
	{
		RecallWidget->RemoveFromParent();
		RecallWidget = nullptr;
	}
}

void UGA_All_Character_Recall::OnRecallPressAgain(float TimeElapsed)
{
	K2_CancelAbility();
}

void UGA_All_Character_Recall::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                          bool bReplicateEndAbility, bool bWasCancelled)
{
	HideRecallWidget();

	Avatar->IsRecalling = false;

	if (ASC && HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
		ASC->RemoveGameplayCue(RecallCueTag);
	
	if (RecallTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(RecallTimerHandle);
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}