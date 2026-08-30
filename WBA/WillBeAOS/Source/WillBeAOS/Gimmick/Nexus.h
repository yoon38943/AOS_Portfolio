#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Character/AOSActor.h"
#include "GameFramework/Actor.h"
#include "Nexus.generated.h"

class UBoxComponent;
class USphereComponent;
class UWAbilitySystemComponent;
class UWAttributeSet;

UCLASS()
class WILLBEAOS_API ANexus : public AAOSActor, public IAbilitySystemInterface
{
	
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "GameEnd")
	UParticleSystem* DestroyParticle;

	UPROPERTY(EditAnywhere, Category = "GameEnd")
	TSubclassOf<AActor> EndingCameraClass;

	
public: //채력 관련
	float GetNexusHPPercent();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly)
	USphereComponent* FakeRootCollision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStaticMeshComponent* NexusMeshComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UCombatComponent* CombatComp;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* EndingCameraComponent;

public:	
	ANexus();

	UStaticMeshComponent* GetMesh() { return NexusMeshComponent; }

	void SetTeamCollision();

	void DestroyNexus();

	UFUNCTION(NetMulticast, reliable)
	void NM_DestroyNexus();

	/*********************************************************/
	// GAS 시스템
	/*********************************************************/
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability")
	UWAbilitySystemComponent* WAbilitySystemComponent;
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability")
	UWAttributeSet* WAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TObjectPtr<UDataTable> StatTable;
	
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TSubclassOf<UGameplayEffect> InitStatEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Ability")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;
};
