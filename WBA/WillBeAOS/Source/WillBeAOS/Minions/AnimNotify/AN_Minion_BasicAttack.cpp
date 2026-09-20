#include "Minions/AnimNotify/AN_Minion_BasicAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Character/WCharacterBase.h"
#include "Minions/WMinionsCharacterBase.h"


void UAN_Minion_BasicAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                         float TotalDuration)
{
	if (!MeshComp->GetOwner()->HasAuthority()) return;
	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	Minion = Cast<AWMinionsCharacterBase>(MeshComp->GetOwner());

	MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	HitActors.Empty();

	PrevMidLocation = MeshComp->GetSocketLocation("Sword_Strike");
}

void UAN_Minion_BasicAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime)
{
	if (!MeshComp->GetOwner()->HasAuthority()) return;
	
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	FVector CurrentLoc = MeshComp->GetSocketLocation("Sword_Strike");
	FVector MuzzleLoc = MeshComp->GetSocketLocation("Muzzle_01");

	FVector BladeDirection = (MuzzleLoc - CurrentLoc).GetSafeNormal();

	FQuat BladeRotation = FRotationMatrix::MakeFromZ(BladeDirection).ToQuat();

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MeshComp->GetOwner());

	ECollisionChannel EnemyCollision;
	if (!Minion) return;
	if (Minion->GetTeamID() == E_TeamID::Blue)
	{
		EnemyCollision = CollisionInfo::RedTeam;
	}
	else
	{
		EnemyCollision = CollisionInfo::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(EnemyCollision);

	MeshComp->GetWorld()->SweepMultiByObjectType(
		HitResults,
		PrevMidLocation,
		CurrentLoc,
		BladeRotation,
		ObjectParams,
		FCollisionShape::MakeSphere(25.f),
		Params
	);

	for (FHitResult Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		IGetInfoInterface* TargetTeam = Cast<IGetInfoInterface>(HitActor);
		IGetInfoInterface* SourceTeam = Cast<IGetInfoInterface>(MeshComp->GetOwner());

		if (!TargetTeam || !SourceTeam) continue;

		HitActors.Add(HitActor);

		FGameplayEventData EventData;
		EventData.Target = HitActor;
		EventData.Instigator = MeshComp->GetOwner();

		FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(Result);
		EventData.TargetData.Add(TargetData);

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), EventTag, EventData);
	}

	PrevMidLocation = CurrentLoc;
}
