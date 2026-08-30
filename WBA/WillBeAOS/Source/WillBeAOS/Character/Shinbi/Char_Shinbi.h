#pragma once

#include "CoreMinimal.h"
#include "Character/WCharacterBase.h"
#include "Character/Skill/SkillDataTable.h"
#include "Char_Shinbi.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQSkillUsed, FString, CharacterName, float, SkillCollTime);

UCLASS()
class WILLBEAOS_API AChar_Shinbi : public AWCharacterBase
{
	GENERATED_BODY()

public:
	AChar_Shinbi();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	TArray<AActor*> GetTartgetInCenter();
};
