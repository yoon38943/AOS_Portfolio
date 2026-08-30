#include "WCharAnimInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "KismetAnimationLibrary.h"
#include "WCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

void UWCharAnimInstance::RegisterTagEvent()
{
	if (!OwnerASC) return;

	OwnerASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag("state.combat"),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UWCharAnimInstance::OnCombatTagChanged);

	BindCheckRecallTag();
}

void UWCharAnimInstance::OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bIsCombat = NewCount > 0;
}

void UWCharAnimInstance::UpdateVelocityData()
{
	if (!WCharBase) return;
	
	// Velocity(속도)
	Velocity2D = CachedVelocity * FVector(1.f,1.f,0.f);
	WCharSpeed = UKismetMathLibrary::VSizeXY(CachedVelocity);

	VelocityLocomotionAngle = UKismetAnimationLibrary::CalculateDirection(Velocity2D, WorldRotation);

	// RootYawOffset
	VelocityLocomotionAngleWithOffset = VelocityLocomotionAngle - RootYawOffset;

	// Acceleration (가속도)
	Acceleration2D = CachedAcceleration * FVector(1.f, 1.f, 0.f);
	if (UKismetMathLibrary::VSizeXY(WCharMovementComponent->GetCurrentAcceleration()) > 0)
	{
		WIsAccelerating = true;
	}
	else
	{
		WIsAccelerating = false;
	}
}

void UWCharAnimInstance::UpdateRotationData(float DeltaSeconds)
{	
	DeltaYawSincelastUpdate = CachedActorRotation.Yaw - WorldRotation.Yaw;
	WorldRotation = CachedActorRotation;

	if (bIsFirstUpdate)
	{
		DeltaYawSincelastUpdate = 0.f;
	}
}

void UWCharAnimInstance::UpdateRootYawOffset(float DeltaTime)
{
	if (RootYawOffsetMode == E_RootYawOffsetMode::Accumulate)
	{
		SetRootYawOffset(RootYawOffset - DeltaYawSincelastUpdate);
	}
	else if (RootYawOffsetMode == E_RootYawOffsetMode::BlendOut)
	{
		TurnYawCurveValue = 0.f;
		float Interp = UKismetMathLibrary::FloatSpringInterp(RootYawOffset, 0.f, RootYawOffsetSpringState, 150.f, 1.f, DeltaTime, 1.f, 0.5f);
		SetRootYawOffset(Interp);
	}

	RootYawOffsetMode = E_RootYawOffsetMode::BlendOut;
}

void UWCharAnimInstance::SetRootYawOffset(float InRootYawOffest)
{
	RootYawOffset = UKismetMathLibrary::NormalizeAxis(InRootYawOffest);
	AimYaw = FMath::Clamp(RootYawOffset * -1, -180.f, 180.f);
}

void UWCharAnimInstance::TurnInPlace(float DeltaTime)
{
	if (FMath::Abs(RootYawOffset) > 50.f && !WIsAccelerating && !bSetForward)
	{
		CurrentTurnDelayTime += DeltaTime;
		if (CurrentTurnDelayTime > TurnDelayThreshold)
		{
			TurnInPlaceMode(-RootYawOffset);
			bIsTurning = true;
		}
	}
	else
	{
		bIsTurning = false;
		CurrentTurnDelayTime = 0.f;
		bSavedMode = false;
		TurningInPlace = E_TurningInPlace::E_NotTurning;
	}
}

void UWCharAnimInstance::TurnInPlaceMode(float DeltaYaw)
{
	if (bSavedMode) return;

	bSavedMode = true;

	TargetTurnYaw = DeltaYaw;
	
	if (DeltaYaw > 120.f)
	{
		TurningInPlace = E_TurningInPlace::E_TurningRight_180;
	}
	else if (DeltaYaw < -120.f)
	{
		TurningInPlace = E_TurningInPlace::E_TurningLeft_180;
	}
	else if (DeltaYaw > 45.f)
	{
		TurningInPlace = E_TurningInPlace::E_TurningRight_90;
	}
	else if (DeltaYaw < -45.f)
	{
		TurningInPlace = E_TurningInPlace::E_TurningLeft_90;
	}
	else
	{
		TurningInPlace = E_TurningInPlace::E_NotTurning;
	}
}

void UWCharAnimInstance::FullBodyUpdate()
{
	if (WCharMovementComponent)
	{		
		// FullBody (풀바디 커브)
		float CurveValue = GetCurveValue(TEXT("FullBody"));
		if (CurveValue > 0.f)
		{
			FullBody = true;
		}
		else
			FullBody = false;
	}
}

void UWCharAnimInstance::NativeInitializeAnimation()
{
	WCharBase = Cast<AWCharacterBase>(TryGetPawnOwner());
	if (!WCharBase) return;
	
	WCharMovementComponent = WCharBase->GetCharacterMovement();

	WCharMesh = WCharBase->GetMesh();
}

void UWCharAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	PropertyAccess();

	if (bShouldResetRootYawOffset)
	{
		ResetRootYawOffset(DeltaSeconds);
	}

	if (!OwnerASC)
	{
		AActor* Owner = GetOwningActor();
		if (Owner)
		{
			OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);

			if (OwnerASC)
			{
				RegisterTagEvent();
			}
		}
	}

	bIsFirstUpdate = false;
}

void UWCharAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateRotationData(DeltaSeconds);
	UpdateVelocityData();
	UpdateRootYawOffset(DeltaSeconds);
	UpdateAimPitch();
	TurnInPlace(DeltaSeconds);
	CaculateLocomotionDirection();
	FullBodyUpdate();
}

void UWCharAnimInstance::PropertyAccess()
{
	if (GetOwningActor()) CachedActorRotation = GetOwningActor()->GetActorRotation();
	if (TryGetPawnOwner())
	{
		CachedVelocity = TryGetPawnOwner()->GetVelocity();
		CachedAimPitch = TryGetPawnOwner()->GetBaseAimRotation().Pitch;
	}
	if (WCharBase) CachedAcceleration = WCharBase->GetCharacterMovement()->GetCurrentAcceleration();
}

void UWCharAnimInstance::CaculateLocomotionDirection()
{
	// DeadZone
	float DeadZone = 20.f;
	
	switch (LocomotionDirection)
	{
	case E_LocomotionDirection::Forward :
		if (FMath::Abs(VelocityLocomotionAngle) < 50.f + DeadZone)
		{
			return;
		}
		break;
	case E_LocomotionDirection::Backward :
		if (VelocityLocomotionAngle < -130.f - DeadZone || VelocityLocomotionAngle > 130.f + DeadZone)
		{
			return;
		}
		break;
	case E_LocomotionDirection::Left :
		if (VelocityLocomotionAngle <= -50.f + DeadZone && VelocityLocomotionAngle >= -130.f - DeadZone)
		{
			return;
		}
		break;
	case E_LocomotionDirection::Right :
		if (VelocityLocomotionAngle >= 50.f - DeadZone && VelocityLocomotionAngle <= 130.f + DeadZone)
		{
			return;
		}
		break;
	}

	if (UKismetMathLibrary::VSizeXY(Velocity2D) < 5.f) return;

	// Setup LocomotionDirection
	if (VelocityLocomotionAngle < -130.f || VelocityLocomotionAngle > 130.f)
	{
		LocomotionDirection = E_LocomotionDirection::Backward;
	}
	else if (VelocityLocomotionAngle >= 50.f && VelocityLocomotionAngle < 130.f)
	{
		LocomotionDirection = E_LocomotionDirection::Right;
	}
	else if (VelocityLocomotionAngle <= -50.f && VelocityLocomotionAngle > -130.f)
	{
		LocomotionDirection = E_LocomotionDirection::Left;
	}
	else
	{
		LocomotionDirection = E_LocomotionDirection::Forward;
	}
}

void UWCharAnimInstance::ProcessTurnYawCurve()
{
	float LastUpdateTurnYawCurveValue = TurnYawCurveValue;

	float TurnRemaining = 0.f;
	bool bHasCurve = GetCurveValue(TEXT("TurnYawWeight"), TurnRemaining);
	if (!bHasCurve)
	{
		TurnYawCurveValue = 0.f;
		LastUpdateTurnYawCurveValue = 0.f;
		return;
	}
	
	if (TurnRemaining < 1.f)
	{		
		TurnYawCurveValue = 0.f;
		LastUpdateTurnYawCurveValue = 0.f;
	}
	else
	{
		float YawCurve = GetCurveValue(TEXT("DistanceCurve"));
		TurnYawCurveValue = UKismetMathLibrary::SafeDivide(YawCurve, TurnRemaining);

		if (TargetTurnYaw >= 0.f)
		{
			TurnYawCurveValue *= -1;
		}

		if (LastUpdateTurnYawCurveValue != 0.f)
		{
			float DeltaTurnYawCurveValue = TurnYawCurveValue - LastUpdateTurnYawCurveValue;
			SetRootYawOffset(RootYawOffset - DeltaTurnYawCurveValue);
		}
	}
}

void UWCharAnimInstance::ResetRootYawOffset(float DeltaTime)
{
	float LerpYaw = FMath::FInterpTo(RootYawOffset, 0.f, DeltaTime, 20.f);
	SetRootYawOffset(LerpYaw);

	ResetTimer += DeltaTime;

	if (ResetTimer >= 0.3f)
	{
		ResetTimer = 0.f;
		SetRootYawOffset(0.f);
		bShouldResetRootYawOffset = false;
	}
}

void UWCharAnimInstance::UpdateAimPitch()
{
	AimPitch = UKismetMathLibrary::NormalizeAxis(CachedAimPitch);
}

void UWCharAnimInstance::BindCheckRecallTag()
{
	if (OwnerASC)
	{
		FGameplayTag RecallTag = FGameplayTag::RequestGameplayTag(FName("ability.state.recall"));
		
		OwnerASC->RegisterGameplayTagEvent(RecallTag, EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(this, &ThisClass::OnRecallTagChanged);

		bIsRecalling = OwnerASC->HasMatchingGameplayTag(RecallTag);
	}
}

void UWCharAnimInstance::OnRecallTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bIsRecalling = NewCount > 0;
}

void UWCharAnimInstance::UpdateSkillLoopingParts(UAnimationAsset* NewLoopingAnimation)
{
	ActivateSkillLoopingParts = NewLoopingAnimation;

	if (NewLoopingAnimation)
	{
		bIsValidLoopingPart = true;
	}
	else
	{
		bIsValidLoopingPart = false;
	}
}

void UWCharAnimInstance::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, RootYawOffset);
}
