#pragma once

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	StartGameWaiting,
	CharacterSelect,
	LoadingPhase,
	InGame,
	GameEnd
};

UENUM(BlueprintType)
enum class E_TeamID : uint8
{
	Neutral		UMETA(DisplayName = "Neutral"),
	Red			UMETA(DisplayName = "Red"),
	Blue		UMETA(DisplayName = "Blue"),
};

UENUM(BlueprintType)
enum class E_CharacterState : uint8
{
	Nothing					UMETA(DisplayName = "Nothing"),
	Attacking				UMETA(DisplayName = "Attacking"),
	GeneralActionState		UMETA(DisplayName = "GeneralActionState"),
	Dead					UMETA(DisplayName = "Dead"),
	Disabled				UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class E_CharacterAction : uint8
{
	Nothing			UMETA(DisplayName = "Nothing"),
	GeneralAction	UMETA(DisplayName = "GeneralAction"),
	LightAttack		UMETA(DisplayName = "LightAttack"),
	StrongAttack	UMETA(DisplayName = "StrongAttack"),
	FallingAttack	UMETA(DisplayName = "FallingAttack"),
	Dodge			UMETA(DisplayName = "Dodge"),
	EnterCombat	UMETA(DisplayName = "EnterCombat"),
	ExitCombat		UMETA(DisplayName = "ExitCombat"),
	Disabled			UMETA(DisplayName = "Disabled"),
};

UENUM(BlueprintType)
enum class E_MovementSpeed : uint8
{
	Walking			UMETA(DisplayName = "Walking"),
	Jogging			UMETA(DisplayName = "Jogging"),
	Sprinting			UMETA(DisplayName = "Sprinting"),
};

UENUM(BlueprintType)
enum class E_GamePlay : uint8
{
	Nothing,
	GameInit,
	PlayerReady,
	ReadyCountdown,
	Gameplaying,
	GameEnded
};

UENUM(BlueprintType)
enum class E_TurningInPlace : uint8
{
	E_NotTurning			UMETA(DisplayName = "NotTurning"),
	E_TurningLeft_90		UMETA(DisplayName = "TurnLeft_90"),
	E_TurningLeft_180		UMETA(DisplayName = "TurnLeft_180"),
	E_TurningRight_90		UMETA(DisplayName = "TurnRight_90"),
	E_TurningRight_180		UMETA(DisplayName = "TurnRight_180")
};

UENUM(BlueprintType)
enum class E_RootYawOffsetMode : uint8
{
	Accumulate,
	BlendOut,
	Hold
};