#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "FBulletTargetData.generated.h"

USTRUCT(BlueprintType)
struct WILLBEAOS_API FBulletTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector TraceStart = FVector::ZeroVector;

	UPROPERTY()
	FVector TraceEnd = FVector::ZeroVector;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << TraceStart;
		Ar << TraceEnd;
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FBulletTargetData> : public TStructOpsTypeTraitsBase2<FBulletTargetData>
{
	enum { WithNetSerializer = true };
};


USTRUCT(BlueprintType)
struct WILLBEAOS_API FBulletContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector MuzzleLoc = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector StrikePoint = FVector::ZeroVector;;
	UPROPERTY(BlueprintReadOnly)
	float Speed = 0.f;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FBulletContext* Duplicate() const override
	{
		FBulletContext* NewContext = new FBulletContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override
	{
		Super::NetSerialize(Ar, Map, bOutSuccess);
		
		Ar << MuzzleLoc;
		Ar << StrikePoint;
		Ar << Speed;

		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FBulletContext> : public TStructOpsTypeTraitsBase2<FBulletContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};