#include "Character/Wraith/Projectile/BulletTargetActor.h"

#include "FBulletTargetData.h"
#include "Abilities/GameplayAbility.h"
#include "Character/WCharacterBase.h"
#include "GameFramework/SpringArmComponent.h"

ABulletTargetActor::ABulletTargetActor()
{
	bReplicates = true;
	ShouldProduceTargetDataOnServer = false;
}

void ABulletTargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	OwningAbility = Ability;
	
	Avatar = Cast<AWCharacterBase>(OwningAbility->GetAvatarActorFromActorInfo());
}

void ABulletTargetActor::ConfirmTargetingAndContinue()
{
	APlayerController* PC = OwningAbility->GetCurrentActorInfo()->PlayerController.Get();
	if (!PC) return;

	FVector ScreenLocation, ScreenDirection;
	FVector2D ScreenCenter;
	int32 ViewportX, ViewportY;

	PC->GetViewportSize(ViewportX, ViewportY);
	ScreenCenter = FVector2D(ViewportX, ViewportY) * 0.5f;

	PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, ScreenLocation, ScreenDirection);

	FVector TraceStart = ScreenLocation;
	FVector TraceEnd = TraceStart + ScreenDirection * (NormalAttackDistance + Avatar->GetCameraBoom()->TargetArmLength);

	FBulletTargetData* NewData = new FBulletTargetData();
	NewData->TraceStart = TraceStart;
	NewData->TraceEnd = TraceEnd;

	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(NewData);

	TargetDataReadyDelegate.Broadcast(Handle);
}
