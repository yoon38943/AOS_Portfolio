#include "Character/Shinbi/AnimNotify/ANS_Shinbi_BasicAttackTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Character/WCharacterBase.h"
#include "Interface/GetInfoInterface.h"


void UANS_Shinbi_BasicAttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp->GetOwner()->HasAuthority()) return;
	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	Player = Cast<AWCharacterBase>(MeshComp->GetOwner());

	MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	HitActors.Empty();

	FVector CurrentStart = MeshComp->GetSocketLocation("Sword_Root");
	FVector CurrentEnd = MeshComp->GetSocketLocation("Sword_Tip");
	PrevMidLocation = (CurrentStart + CurrentEnd) * 0.5f;
}

void UANS_Shinbi_BasicAttackTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp->GetOwner()->HasAuthority()) return;

	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	FVector CurrentStart = MeshComp->GetSocketLocation("Sword_Root");
	FVector CurrentEnd = MeshComp->GetSocketLocation("Sword_Tip");

	FVector CurrentMidLocation = (CurrentStart + CurrentEnd) * 0.5f;

	FVector BladeDirection = (CurrentStart - CurrentEnd).GetSafeNormal();

	FQuat BladeRotation = FRotationMatrix::MakeFromZ(BladeDirection).ToQuat();

	float BladeLength = FVector::Distance(CurrentStart, CurrentEnd);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MeshComp->GetOwner());

	ECollisionChannel EnemyCollision;
	if (!Player) return;
	if (Player->GetTeamID() == E_TeamID::Blue)
	{
		EnemyCollision = TeamCollision::RedTeam;
	}
	else
	{
		EnemyCollision = TeamCollision::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(EnemyCollision);
	//ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	MeshComp->GetWorld()->SweepMultiByObjectType(
		HitResults,
		PrevMidLocation,
		CurrentMidLocation,
		BladeRotation,
		ObjectParams,
		FCollisionShape::MakeCapsule(20.f, BladeLength * 0.5f),
		Params);

	/*DrawDebugCapsule(
		MeshComp->GetWorld(),
		CurrentMidLocation,
		BladeLength * 0.5f,
		15.f,
		BladeRotation,
		HitResults.Num() > 0 ? FColor::Red : FColor::Green,
		false,
		2.f
	);*/

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

	PrevMidLocation = CurrentMidLocation;
}

void UANS_Shinbi_BasicAttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp->GetOwner()->HasAuthority()) return;

	Super::NotifyEnd(MeshComp, Animation);

	HitActors.Empty();
}
