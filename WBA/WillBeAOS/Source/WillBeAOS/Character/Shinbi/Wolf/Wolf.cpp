#include "Character/Shinbi/Wolf/Wolf.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AOSActor.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"


AWolf::AWolf()
{
	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);
	
	PrimaryActorTick.bCanEverTick = true;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void AWolf::LaunchWolf(AWCharacterBase* InInstigator)
{
	WolfInstigator = InInstigator;
	
	PrevLocation = GetActorLocation();
}

void AWolf::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDisappear) return;

	DashToForward(DeltaTime);
	
	if (HasAuthority())
	{
		CheckPathHit();
	}
}

void AWolf::DashToForward(float DeltaTime)
{
	if (TotalMoveDistance >= DashEndDistance)
	{
		Explosion(GetActorLocation());
		return;
	}
	
	TotalMoveDistance += DashSpeed * DeltaTime;
	FVector NewLocation = GetActorLocation() + (GetActorForwardVector() * DashSpeed * DeltaTime);
	SetActorLocation(NewLocation);
}

void AWolf::Explosion(const FVector& ImpactLocation)
{
	Disappear();

	Explode_Particle_Multicast(ImpactLocation);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(WolfInstigator);

	ECollisionChannel EnemyCollision;
	if (WolfInstigator->GetTeamID() == E_TeamID::Blue)
	{
		EnemyCollision = TeamCollision::RedTeam;
	}
	else
	{
		EnemyCollision = TeamCollision::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(EnemyCollision);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		ImpactLocation,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(ExplosionRadius),
		Params
	);

	//DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 16, Overlaps.Num() > 0 ? FColor::Green : FColor::Red, false, 3.f);

	for (const auto& Overlap : Overlaps)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Overlap.GetActor()->GetName());
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		IGetInfoInterface* TargetTeam = Cast<IGetInfoInterface>(HitActor);
		IGetInfoInterface* SourceTeam = Cast<IGetInfoInterface>(GetInstigator());

		if (!TargetTeam || !SourceTeam) continue;
		//if (TargetTeam->GetTeamID() == SourceTeam->GetTeamID()) continue;

		FHitResult HitResult;
		ApplyDamageToTarget(HitActor, HitResult, true);
	}
}

void AWolf::Explode_Particle_Multicast_Implementation(const FVector& ImpactLocation)
{
	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		ExplodeParticle,
		ImpactLocation,
		GetActorRotation()
	);
}

void AWolf::Disappear_Implementation()
{
	bIsDisappear = true;
	
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);

	GetMesh()->SetVisibility(false);
	
	if (TrailEffect)
		TrailEffect->DeactivateSystem();

	SetLifeSpan(3.f);
}

void AWolf::CheckPathHit()
{	
	FVector CurrentLocation = GetActorLocation();

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(WolfInstigator);

	ECollisionChannel EnemyCollision;
	if (WolfInstigator->GetTeamID() == E_TeamID::Blue)
	{
		EnemyCollision = TeamCollision::RedTeam;
	}
	else
	{
		EnemyCollision = TeamCollision::BlueTeam;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(EnemyCollision);

	GetWorld()->SweepMultiByObjectType(
		HitResults,
		PrevLocation,
		CurrentLocation,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeBox(FVector(70, 20, 40)),
		Params
	);

	/*DrawDebugBox(
		GetWorld(),
		CurrentLocation,
		FVector(70, 20, 40),
		GetActorForwardVector().ToOrientationQuat(),
		HitResults.Num() > 0 ? FColor::Green : FColor::Red,
		false,
		5.0f
	);*/

	for (FHitResult HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		IGetInfoInterface* TargetTeam = Cast<IGetInfoInterface>(HitActor);
		IGetInfoInterface* SourceTeam = Cast<IGetInfoInterface>(GetInstigator());

		if (!TargetTeam || !SourceTeam) continue;
		//if (TargetTeam->GetTeamID() == SourceTeam->GetTeamID()) continue;

		HitActors.Add(HitActor);

		if (Cast<AWCharacterBase>(HitActor) || Cast<AAOSActor>(HitActor))
		{
			Explosion(HitResult.ImpactPoint);
			return;
		}

		ApplyDamageToTarget(HitActor, HitResult, false);
	}

	PrevLocation = CurrentLocation;
}

void AWolf::ApplyDamageToTarget(AActor* HitActor, FHitResult& HitResult, bool bExplosion)
{
	if (!HitActor) return;

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WolfInstigator);
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(WolfInstigator, this);
	if (!bExplosion) EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Shinbi_QSkill_DamageEffect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		2.5f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}