#pragma once

#include "CoreMinimal.h"
#include "WEnumFile.h"
#include "GameFramework/Actor.h"
#include "SpawnTowerPoint.generated.h"

UCLASS()
class WILLBEAOS_API ASpawnTowerPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnTowerPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USceneComponent* Root;
	
	UPROPERTY(EditAnywhere, Category = "Component")
	class UStaticMeshComponent* PointMesh;

	// 타워 팀 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	E_TeamID TeamID;

	// 타워 타입 (예: 방어 타워, 공격 타워 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> TowerClass;	
};
