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
	
public:	//타격 관련
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	
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
};
