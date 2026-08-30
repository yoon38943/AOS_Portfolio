#include "WCharacterBase.h"
#include "GAS/WAbilitySystemComponent.h"
#include "GAS/WAttributeSet.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CombatComponent.h"
#include "WCharAnimInstance.h"
#include "Component/VisibleWidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ProgressBar.h"
#include "Components/SceneComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Gimmick/Tower.h"
#include "Minions/WMinionsCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "PersistentGame/GamePlayerController.h"
#include "PersistentGame/GamePlayerState.h"
#include "PersistentGame/PlayGameMode.h"
#include "PersistentGame/PlayGameState.h"
#include "UI/PlayerHPInfoBar.h"


AWCharacterBase::AWCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComp->SetCombatEnable(true);

	HPInfoBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPInfoBar"));
	HPInfoBarComponent->SetupAttachment(GetRootComponent());
	HPInfoBarComponent->SetRelativeLocation(FVector(0.f, 0.f, BaseWidgetHeight));
	
	SightComp = CreateDefaultSubobject<UVisibleWidgetComponent>(TEXT("SightComponent"));

	SetReplicateMovement(true);
	bAlwaysRelevant = true;

	SetGoldReward(PLAYERKILLGOLD);

	bReplicates = true;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->NetworkSmoothingMode = ENetworkSmoothingMode::Linear;
		MoveComp->NetworkSimulatedSmoothRotationTime = 0.1f;
	}
}

void AWCharacterBase::InitialAbilitySystem()
{
	GamePlayerState = GetPlayerState<AGamePlayerState>();
	if (!GamePlayerState) return;

	AbilitySystemComponent = GamePlayerState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(GamePlayerState, this);

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (UWCharAnimInstance* MainAnim = Cast<UWCharAnimInstance>(AnimInstance))
		{
			MainAnim->OnUpdatedASC();
		}
	}

	if (!HasAuthority())
	{
		ConfigureOverHeadHealthWidget();
	}
}

void AWCharacterBase::ServerSideInit()
{
	if (!GamePlayerState || !AbilitySystemComponent) return;
	
	AbilitySystemComponent->ApplyInitialStat(StatTable, InitStatEffect, CharacterName);
	AbilitySystemComponent->ApplyInitialEffects(InitialEffects);
	AbilitySystemComponent->GiveInitialAbilities(Abilities, BasicAbilities);
	AbilitySystemComponent->SetIsNotStartGame();

	RegisterTagEvent();
}

void AWCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitialAbilitySystem();
	ServerSideInit();
}

void AWCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitialAbilitySystem();
	RegisterTagEvent();
}

void AWCharacterBase::RegisterTagEvent()
{
	AbilitySystemComponent->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag("state.combat"),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AWCharacterBase::OnCombatTagChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("ability.state.recall")),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AWCharacterBase::OnRecallTagChanged);
}

void AWCharacterBase::OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bIsCombat = (NewCount > 0);

	if (NewCount > 0)
	{
		GetMesh()->LinkAnimClassLayers(Combat_Layer);
	}
	else
	{
		if (!AbilitySystemComponent->HasAnyMatchingGameplayTags(MaintainCombatTags))
		{
			GetMesh()->LinkAnimClassLayers(NonCombat_Layer);
		}
	}
}

void AWCharacterBase::OnRecallTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(RecallZoomTimer, this, &ThisClass::UpdateRecallZoom, 0.01f, true);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(RecallZoomTimer, this, &ThisClass::UpdateRecallZoom, 0.01f, true);		
	}
}

void AWCharacterBase::HandleAbilityInputPressed(const FInputActionValue& Value, EWAbilityInputID InputID)
{
	GetAbilitySystemComponent()->AbilityLocalInputPressed((int32)InputID);
}

void AWCharacterBase::HandleAbilityInputReleased(const FInputActionValue& Value, EWAbilityInputID InputID)
{
	GetAbilitySystemComponent()->AbilityLocalInputReleased((int32)InputID);
}

UAbilitySystemComponent* AWCharacterBase::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent)
		return AbilitySystemComponent;
	return nullptr;
}

void AWCharacterBase::MoveDecalToCameraForward()
{
	if (SkillForwardDecal && IsLocallyControlled())
	{
		FRotator ControlRotation = GetControlRotation();
		FRotator DecalRotation = FRotator(-90.f, ControlRotation.Yaw + 90.f, 0.f);
		SkillForwardDecal->SetWorldRotation(DecalRotation);

		FVector ForwardXY = FRotator(0.f, ControlRotation.Yaw, 0.f).Vector();
		FVector NewLocation = GetActorLocation() + ForwardXY * 700.f;
		SkillForwardDecal->SetWorldLocation(NewLocation);
	}
}

void AWCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	APlayGameMode* GM = Cast<APlayGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		// 게임 종료 델리게이트 바인딩 ( 서버에서만 일어남 )
		GM->OnGameEnd.AddUObject(this, &ThisClass::HandleGameEnd);
	}
	
	if (!HasAuthority())
	{
		SetHPInfoBarColor();
		ShowNickName();
	}
	else
	{
		APlayGameState* GS = Cast<APlayGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->GameManagedActors.AddUnique(this);
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		Anim = Cast<UWCharAnimInstance>(AnimInstance);
	}

	SetTeamCollision();
}

void AWCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AWCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateAcceleration();

	VisibleOutline();

	if (IsLocallyControlled())
		MoveDecalToCameraForward();
}

void AWCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// IMC 세팅
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(IMC_Asset, 0);
		}
	}

	// InputAction 붙이기
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AWCharacterBase::Look);
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AWCharacterBase::Move);
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Completed, this, &AWCharacterBase::StopMove);

		for (const TPair<EWAbilityInputID, UInputAction*>& InputActionPair : GameplayAbilityInputActions)
		{
			EnhancedInputComponent->BindAction(InputActionPair.Value, ETriggerEvent::Started, this, &AWCharacterBase::HandleAbilityInputPressed, InputActionPair.Key);
			EnhancedInputComponent->BindAction(InputActionPair.Value, ETriggerEvent::Completed, this, &AWCharacterBase::HandleAbilityInputReleased, InputActionPair.Key);
		}
	}
}

UWAbilitySystemComponent* AWCharacterBase::GetGameplayerStateASC() const
{
	if (AGamePlayerState* PS = GetPlayerState<AGamePlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

void AWCharacterBase::SetTeamCollision()
{
	//팀 정보는 GameMode에서 리스폰시킬 때 넣고 있음
	if (GetTeamID() == E_TeamID::Blue)
	{
		TeamTraceCollision->SetCollisionObjectType(TeamCollision::BlueTeam);
	}
	if (GetTeamID() == E_TeamID::Red)
	{
		TeamTraceCollision->SetCollisionObjectType(TeamCollision::RedTeam);
	}
}

void AWCharacterBase::RequestSnapToCameraDirection(float SnapDirection)
{
	if (Anim && FMath::Abs(Anim->RootYawOffset) > SnapDirection)
	{
		Anim->bIsTurning = false;
		Anim->bShouldResetRootYawOffset = true;
	}
}

void AWCharacterBase::SetHPInfoBarColor()
{
	if (GamePlayerState && GamePlayerState->PlayerInfo.PlayerTeam != E_TeamID::Neutral)
	{
		UPlayerHPInfoBar* HPInfoBar = Cast<UPlayerHPInfoBar>(HPInfoBarComponent->GetWidget());
		if (HPInfoBar)
		{
			if (IsLocallyControlled())
			{
				HPInfoBarColor = SelfHPColor;
			
				HPInfoBar->PlayerHPBar->SetFillColorAndOpacity(HPInfoBarColor);

				HPInfoBar->InvalidateLayoutAndVolatility();
			}
			else
			{
				if (GamePlayerState->PlayerInfo.PlayerTeam == E_TeamID::Blue)
				{
					HPInfoBarColor = BlueTeamHPColor; 
				}
				else if (GamePlayerState->PlayerInfo.PlayerTeam == E_TeamID::Red)
				{
					HPInfoBarColor = RedTeamHPColor;
				}
			
				HPInfoBar->PlayerHPBar->SetFillColorAndOpacity(HPInfoBarColor);

				HPInfoBar->InvalidateLayoutAndVolatility();
			}
		}
	}
	else
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &ThisClass::SetHPInfoBarColor, 0.1f, false);
	}
}

void AWCharacterBase::ConfigureOverHeadHealthWidget()
{
	if (!HPInfoBarComponent)
	{
		return;
	}

	if (IsLocallyControlled())
	{
		HPInfoBarComponent->SetHiddenInGame(true);
	}

	UPlayerHPInfoBar* HpInfoBar = Cast<UPlayerHPInfoBar>(HPInfoBarComponent->GetWidget());
	if (HpInfoBar)
	{
		HpInfoBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent, UWAttributeSet::GetHealthAttribute(), UWAttributeSet::GetMaxHealthAttribute());
	}
}

void AWCharacterBase::ShowNickName()
{
	if (GamePlayerState)
	{
		if (GamePlayerState->PlayerInfo.PlayerNickName == "")
		{
			FTimerHandle Handle;
			GetWorld()->GetTimerManager().SetTimer(Handle, this, &ThisClass::ShowNickName, 0.1f, false);
		}
		else
		{
			if (UPlayerHPInfoBar* HPInfoBar = Cast<UPlayerHPInfoBar>(HPInfoBarComponent->GetWidget()))
			{
				HPInfoBar->PlayerNickName->SetText(FText::FromString(GamePlayerState->PlayerInfo.PlayerNickName));
			}
		}
	}
	else
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &ThisClass::ShowNickName, 0.1f, false);
	}
}

void AWCharacterBase::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X * MouseSensitivityMultiply);
		AddControllerPitchInput(LookAxisVector.Y * MouseSensitivityMultiply);
	}
}

void AWCharacterBase::Move(const FInputActionValue& Value)
{
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("ability.state.movement.blocked")))
		return;

	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(FGameplayTag::RequestGameplayTag("ability.state.recall"));
	GetAbilitySystemComponent()->CancelAbilities(&CancelTags);
	
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRatator(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRatator).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRatator).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AWCharacterBase::StopMove(const FInputActionValue& Value)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
	}
}

void AWCharacterBase::VisibleOutline()
{
	if (!HasAuthority() && IsLocallyControlled())
	{
		// 캐릭터 죽었을 경우 현재 타겟 모두 없애기
		if (bIsDead)
		{
			if (AttackTarget.Num() > 0)
			{
				TArray<AActor*> DeleteActors;
				for (auto& Actor : AttackTarget)
				{
					DeleteActors.Add(Actor);
				}

				for (auto& Actor : DeleteActors)
				{
					AttackTarget.Remove(Actor);
				}
			}
		}

		// 타겟에 변화가 일어 났을 때
		if (AttackTarget != CurrentTarget)
		{
			// 타겟이 줄었을 때
			if (CurrentTarget.Num() > 0)
			{
				// 타겟이 아니게된 액터 찾아서 외곽선 끄기
				for (auto& Actor : CurrentTarget)
				{
					if (Actor && !AttackTarget.Contains(Actor))
					{
						auto EnemyMesh = Actor->FindComponentByClass<UMeshComponent>();
						if (EnemyMesh)
						{
							EnemyMesh->SetRenderCustomDepth(false);
						}
					}
				}
			}

			// 타겟이 늘었을 때
			if (AttackTarget.Num() > 0)
			{
				// 새로 타겟이 된 액터 찾아서 외곽선 표시하기
				for (auto& Actor : AttackTarget)
				{
					if (Actor && !CurrentTarget.Contains(Actor))
					{
						auto EnemyMesh = Actor->FindComponentByClass<UMeshComponent>();
						if (EnemyMesh)
						{
							EnemyMesh->SetRenderCustomDepth(true);
						}
					}
				}
			}

			CurrentTarget = AttackTarget;	// 외곽선 표시처리 후 배열 동일하게 처리
		}
	}
}

void AWCharacterBase::UpdateMovementSpeedData(float Multiplier)
{
	if (GamePlayerState)
	{
		// 속도
		float CaculatedWalkSpeed = MovementSpeedData.MaxWalkSpeed * GamePlayerState->ItemSpeed;
		float FinalSpeed = CaculatedWalkSpeed * Multiplier;
		GetCharacterMovement()->MaxWalkSpeed = FinalSpeed;

		// 가속도
		float CaculatedAcceleration = MovementSpeedData.MaxAcceleration * GamePlayerState->ItemSpeed;
		float FinalAcceleration = CaculatedAcceleration * Multiplier;
		GetCharacterMovement()->MaxAcceleration = FinalAcceleration;

		// 제동력
		float CaculatedBrakingDeceleration = MovementSpeedData.BrakingDeceleration * GamePlayerState->ItemSpeed;
		float FinalBrakingDeceleration = CaculatedBrakingDeceleration * Multiplier;
		GetCharacterMovement()->BrakingDecelerationWalking = FinalBrakingDeceleration;

		// BrakingFrictionFactor
		GetCharacterMovement()->BrakingFrictionFactor = MovementSpeedData.BrakingFrictionFactor;

		// BrakingFriction
		GetCharacterMovement()->BrakingFriction = MovementSpeedData.BrakingFriction;
	}
}

/*void AWCharacterBase::NM_StopPlayMontage_Implementation()
{
	StopAnimMontage();
}*/

void AWCharacterBase::Server_EnterCombat_Implementation()
{
	if (!IsCombat)	IsCombat = true;
	
	GetWorld()->GetTimerManager().ClearTimer(CombatTimer);
	GetWorld()->GetTimerManager().SetTimer(CombatTimer, this, &ThisClass::ServerExitCombat, 12.f, false);

	ServerChangeCombatMode(IsCombat);
}

void AWCharacterBase::UpdateRecallZoom()
{
	if (IsLocallyControlled())
	{
		float TargetFOV = IsRecalling ? 60.f : 90.f;
		float InterpSpeed = IsRecalling ? 0.4f : 15.f;
		float CurrentFOV = FollowCamera->FieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, GetWorld()->DeltaTimeSeconds, InterpSpeed);

		FollowCamera->SetFieldOfView(NewFOV);

		if (FMath::Abs(NewFOV - TargetFOV) <= 0.1f)
		{
			FollowCamera->SetFieldOfView(TargetFOV);
			GetWorld()->GetTimerManager().ClearTimer(RecallZoomTimer);
		}
	}
}

void AWCharacterBase::ServerExitCombat()
{
	IsCombat = false;

	ServerChangeCombatMode(IsCombat);
}

void AWCharacterBase::ServerChangeCombatMode(bool isCombat)
{
}

void AWCharacterBase::OnRep_QSkillUsing()
{
}

void AWCharacterBase::OnRep_ESkillUsing()
{
}

void AWCharacterBase::Input_QSkill(const FInputActionValue& Value)
{
	Handle_UseSkillButton(ESkillSlot::Q);
}

void AWCharacterBase::Input_ESkill(const FInputActionValue& Value)
{
	Handle_UseSkillButton(ESkillSlot::E);
}

void AWCharacterBase::Input_RSkill(const FInputActionValue& Value)
{
	Handle_UseSkillButton(ESkillSlot::R);
}

void AWCharacterBase::Handle_UseSkillButton(ESkillSlot Skillslot)
{
	if (!IsLocallyControlled()) return;
	
	// 자식 함수에서 정의
}

void AWCharacterBase::UpdateAcceleration()
{
	float CurrentSpeed = GetCharacterMovement()->Velocity.Size();
	float MaxSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// 현재 속도에 비례해서 가속도를 조정 (최대 속도가 높을수록 가속도 증가)
	float NewAcceleration = FMath::Lerp(2048.0f, 5000.0f, CurrentSpeed / MaxSpeed);
	GetCharacterMovement()->MaxAcceleration = NewAcceleration;
}

void AWCharacterBase::Server_SetControlRotationYaw_Implementation(FRotator YawRotation)
{
	SetActorRotation(YawRotation);	// 서버에서 캐릭터 회전값 동기화
}

bool AWCharacterBase::Server_SetControlRotationYaw_Validate(FRotator YawRotation)
{
	return true;
}

void AWCharacterBase::RecallAbilityInputPressed(const FInputActionValue& Value,
	TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->TryActivateAbilityByClass(AbilityClass);
	}
}

void AWCharacterBase::MultiPlayMontage_Implementation(UAnimMontage* Montage)
{
	if (Montage)
	{
		PlayAnimMontage(Montage);
	}
}

void AWCharacterBase::SpawnHitEffect_Implementation(FVector HitLocation)
{
}

void AWCharacterBase::NM_SpawnHitEffect_Implementation(FVector HitLocation)
{
	SpawnHitEffect(HitLocation);
}

void AWCharacterBase::ActivateSkill_Implementation(ESkillSlot SkillSlot)
{
	// 자식 캐릭터 클래스에서 채움
}

void AWCharacterBase::ExecuteSkill(ESkillSlot SkillSlot)
{
	// 자식 캐릭터 클래스에서 채움
}

void AWCharacterBase::BeingDead()
{
	bIsDead = true;

	AGamePlayerController* PC = Cast<AGamePlayerController>(GetController());
	
	C_BeingDead(PC);	// 클라에서 실행하는 것
	S_BeingDead(PC, this);	// 서버에서 실행하는 것
}

void AWCharacterBase::S_BeingDead_Implementation(AGamePlayerController* PC, APawn* Player)
{
	bIsDead = true;

	// 골드
	APlayGameMode* GameMode = Cast<APlayGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->OnObjectKilled(this, LastHitBy);
	}
	
	//캐릭터 리스폰
	APlayGameState* GameState = Cast<APlayGameState>(GetWorld()->GetGameState());
	if (PC && GameState && GameMode && HasAuthority())
	{
		PC->S_SetCurrentRespawnTime();

		PC->S_CountRespawnTime();
		
		GameState->GameManagedActors.Remove(Player);
		
		FTimerHandle RespawnTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle,
			[Player, PC, GameMode]()
			{
				if (PC)
				{
					GameMode->RespawnPlayer(Player, PC);
				}
			}, GameState->RespawnTime, false);

		FTimerHandle SpectatorCamera;
		GetWorld()->GetTimerManager().SetTimer(SpectatorCamera,
			[this, PC]()
			{
				PC->PossessToSpectatorCamera(FollowCamera->GetComponentLocation(), FollowCamera->GetComponentRotation());
			}, 1.3f, false);
	}
	
	NM_BeingDead();
}

void AWCharacterBase::NM_BeingDead_Implementation()
{
	//무브먼트, 콜리전 없애고 몽타주 출력
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	PlayAnimMontage(DeadAnimMontage);

	SetLifeSpan(1.3f);
}

void AWCharacterBase::C_BeingDead_Implementation(AGamePlayerController* PC)
{
	// CheckDistance 셋타이머 끄기
	if (CheckTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
	}
	
	// 클라에서 리스폰 위젯 출력
	if (PC != nullptr)
	{
		//리스폰 실행
		PC->ShowRespawnWidget();
	}

	/*//죽으면 카메라 움직임에 메쉬 따라 움직이지 않게 하기
	this->bUseControllerRotationYaw = false;*/

	// 죽으면 카메라 회전 못하게 하기
	PC->SetIgnoreLookInput(true);
}

//포인트 데미지 주는 함수
void AWCharacterBase::HandleApplyPointDamage(FHitResult LastHit)
{
	if (HasAuthority())
	{
		NM_SpawnHitEffect(LastHit.Location);
		
		AGamePlayerController* PC = Cast<AGamePlayerController>(GetController());
		if (PC)
		{
			AGamePlayerState* PState = PC->GetPlayerState<AGamePlayerState>();
			if (PState)
			{
				CharacterDamage = PState->CPower;
			}
		}
		
		UGameplayStatics::ApplyPointDamage(
			LastHit.GetActor(),
			CharacterDamage,
			GetOwner()->GetActorForwardVector(),
			LastHit,
			GetInstigatorController(),
			this,
			UDamageType::StaticClass()
		);
	}
}

// 데미지 받는 함수
float AWCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority())
	{
		Server_EnterCombat();

		FTimerHandle SaveDamagedByEnemyTimer;
		if (Cast<AWCharacterBase>(DamageCauser))
		{
			LastHitBy = EventInstigator;

			if (SaveDamagedByEnemyTimer.IsValid())
			{
				GetWorld()->GetTimerManager().ClearTimer(SaveDamagedByEnemyTimer);
			}
			
			GetWorld()->GetTimerManager().SetTimer(SaveDamagedByEnemyTimer, this, &ThisClass::ClearLastHitBy, 7.f, false);
		}
		
		if (GamePlayerState)
		{
			GamePlayerState->Server_ApplyDamage(DamageAmount, LastHitBy, DamageCauser);
		}
	}

	AWCharacterBase* AttackChar = Cast<AWCharacterBase>(DamageCauser);
	if (TowerWithCharacterInside && AttackChar && TowerWithCharacterInside->OverlappingActors.Contains(AttackChar))
	{
		TowerWithCharacterInside->OverlappingActors.Swap(0, TowerWithCharacterInside->OverlappingActors.Find(AttackChar));
	}
	
	return DamageAmount;
}

void AWCharacterBase::ClearLastHitBy()
{
	LastHitBy = nullptr;
}

void AWCharacterBase::HandleGameEnd()
{
	GetCharacterMovement()->DisableMovement();

	if (IsLocallyControlled())
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC) PC->DisableInput(PC);

		PrimaryActorTick.bCanEverTick = false;
	}
}

void AWCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CharacterDamage);
	DOREPLIFETIME(ThisClass, IsCombat);
	DOREPLIFETIME(ThisClass, IsRecalling);
}