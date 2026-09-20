#include "Character/Shinbi/DashHitCollision.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AOSActor.h"
#include "Gimmick/Nexus.h"
#include "Gimmick/Tower.h"
#include "Interface/GetInfoInterface.h"


ADashHitCollision::ADashHitCollision()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADashHitCollision::InitCollision(AActor* InOwner, float InRadius, float InHalfHeight)
{
	SkillOwner = InOwner;
	Radius = InRadius;
	HalfHeight = InHalfHeight;
	PrevLocation = GetActorLocation();

	Player = Cast<AWCharacterBase>(InOwner);

	bCanCheck = true;
}

void ADashHitCollision::BeginPlay()
{
	Super::BeginPlay();
}

void ADashHitCollision::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLocation = SkillOwner->GetActorLocation();
	SetActorLocation(CurrentLocation);

	if (!bCanCheck) return;

	if (HasAuthority())
	{
		CheckHitPath();
	}

	PrevLocation = CurrentLocation;
}

void ADashHitCollision::CheckHitPath()
{
	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(SkillOwner);

	ECollisionChannel EnemyCollision;
	if (Player->GetTeamID() == E_TeamID::Blue)
	{
		EnemyCollision = CollisionInfo::RedTeam;
	}
	else
	{
		EnemyCollision = CollisionInfo::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(EnemyCollision);

	bool bHit = GetWorld()->SweepMultiByObjectType(
		HitResults,
		PrevLocation,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeCapsule(Radius, HalfHeight),
		Params
	);

	if (!bHit) return;

	TMap<AActor*, FHitResult> TickBestHits;

	for (auto& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		const IGetInfoInterface* TargetTeam = Cast<IGetInfoInterface>(HitActor);
		const IGetInfoInterface* SourceTeam = Cast<IGetInfoInterface>(SkillOwner);
		if (!TargetTeam || !SourceTeam) continue;

		HitActors.Add(HitActor);
		ApplyDamageToTarget(HitActor, HitResult);
	}
}

FVector ADashHitCollision::GetClosestPoint(AActor* HitActor)
{
	UPrimitiveComponent* TargetComp;
	if (ANexus* Nexus = Cast<ANexus>(HitActor))
		TargetComp = Nexus->GetMesh();
	else if (ATower* Tower = Cast<ATower>(HitActor))
		TargetComp = Tower->GetMesh();
	else
		TargetComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
	
	FVector ClosestPoint;
	if (TargetComp)
	{
		FVector DashDirection = (GetActorLocation() - PrevLocation).GetSafeNormal();
		TargetComp->GetClosestPointOnCollision(GetActorLocation() - (DashDirection * 50), ClosestPoint);
		return ClosestPoint;
	}
	return GetActorLocation();
}

void ADashHitCollision::ApplyDamageToTarget(AActor* HitActor, FHitResult& HitResult)
{
	if (!HitActor) return;

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SkillOwner);
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(SkillOwner, this);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Shinbi_RMSkill_DamageEffect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		1.5f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

