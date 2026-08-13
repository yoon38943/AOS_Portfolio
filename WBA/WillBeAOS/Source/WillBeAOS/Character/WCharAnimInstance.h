#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstance.h"
#include "WEnumFile.h"
#include "Kismet/KismetMathLibrary.h"
#include "Struct_Enum/E_LocomotionDirection.h"
#include "WCharAnimInstance.generated.h"

class UAbilitySystemComponent;
class AWCharacterBase;
class UCharacterMovementComponent;

UCLASS()
class WILLBEAOS_API UWCharAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, Category = AnimInstance)
	AWCharacterBase* WCharBase;
	UPROPERTY(BlueprintReadOnly, Category = AnimInstance)
	UCharacterMovementComponent* WCharMovementComponent;
	UPROPERTY(BlueprintReadOnly, Category = AnimInstance)
	UMeshComponent* WCharMesh;

	UPROPERTY(BlueprintReadOnly)
	bool bIsFirstUpdate = true;
	
	UPROPERTY(BlueprintReadOnly, Category = "RotationData")
	FRotator WorldRotation;

	UPROPERTY(BlueprintReadOnly, Category = "RotationData")
	float DeltaYawSincelastUpdate = 0.f;
	
	
	UPROPERTY(BlueprintReadOnly, Category = "ValocityData")
	FVector Velocity2D;

	UPROPERTY(BlueprintReadOnly, Category = "ValocityData")
	float VelocityLocomotionAngle = 0.f;
	

	UPROPERTY(BlueprintReadOnly, Category = "RootYawOffset")
	float VelocityLocomotionAngleWithOffset = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "RootYawOffset")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "RootYawOffset")
	E_RootYawOffsetMode RootYawOffsetMode;

	

	FFloatSpringState RootYawOffsetSpringState;

	UFUNCTION(Category = "ValocityData", meta = (BlueprintThreadSafe))
	void UpdateVelocityData();
	UFUNCTION(Category = "RotationData", meta = (BlueprintThreadSafe))
	void UpdateRotationData(float DeltaSeconds);
	UFUNCTION(Category = "RootYawOffset")
	void UpdateRootYawOffset(float DeltaTime);

	void SetRootYawOffset(float InRootYawOffest);

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	E_TurningInPlace TurningInPlace;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float CurrentYawDelta = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float TargetTurnYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float TurnDistance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	bool bIsTurning = false;

	float TurnDelayThreshold = 0.2f;
	float CurrentTurnDelayTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	bool bSavedMode = false;

	UFUNCTION(meta = (BlueprintTreadSafe))
	void TurnInPlace(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "TurnInPlace", meta = (BlueprintThreadSafe))
	void TurnInPlaceMode(float DeltaYaw);

	UFUNCTION(meta = (BlueprintThreadSafe))
	void CaculateLocomotionDirection();

	UPROPERTY(BlueprintReadWrite, Category = "TurnInPlace")
	float TurnYawCurveValue = 0.f;

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	void ProcessTurnYawCurve();
	

protected:

	UPROPERTY(BlueprintReadOnly, Category = "AimOffset")
	float AimPitch;
	UPROPERTY(BlueprintReadOnly, Category = "AimOffset")
	float AimYaw;

	UFUNCTION(meta = (BlueprintThreadSafe))
	void UpdateAimPitch();

	UPROPERTY(BlueprintReadOnly)
	bool bIsRecalling;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float WCharSpeed;
	UPROPERTY(BlueprintReadOnly, Category = Acceleration)
	FVector Acceleration2D;
	UPROPERTY(BlueprintReadOnly, Category = Acceleration)
	bool WIsAccelerating;
	UPROPERTY(BlueprintReadOnly, Category = "FullBody")
	bool FullBody;

	UFUNCTION(meta = (BlueprintTreadSafe))
	void FullBodyUpdate();
	
	UPROPERTY(BlueprintReadOnly)
	E_LocomotionDirection LocomotionDirection;

	UPROPERTY(BlueprintReadOnly)
	bool bIsCombat = false;

	UPROPERTY(BlueprintReadOnly)
	UAbilitySystemComponent* OwnerASC;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// 스레드에서 사용할 캐싱된 변수 작업
	void PropertyAccess();

	FRotator CachedActorRotation;
	FVector CachedVelocity;
	FVector CachedAcceleration;
	float CachedAimPitch;

	// 전투 모드
	void RegisterTagEvent();

	UFUNCTION()
	void OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 슈팅 모드
	UFUNCTION(BlueprintCallable, Category = "Animation|ShootingMode", meta = (BlueprintThreadSafe))
	virtual FGameplayTag GetCurrentShootingModeTag() const { return FGameplayTag(); }

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
