#include "Character/Shinbi/DashHitCollision.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeCapsule(Radius, HalfHeight),
		Params
	);

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

void ADashHitCollision::ApplyDamageToTarget(AActor* HitActor, FHitResult& HitResult)
{
	if (!HitActor) return;

	UE_LOG(LogTemp, Display, TEXT("Apply Damage1"));

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SkillOwner);
	if (!SourceASC) return;

	UE_LOG(LogTemp, Display, TEXT("Owner : %s"), *SkillOwner->GetName());

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	UE_LOG(LogTemp, Display, TEXT("Apply Damage3"));

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(SkillOwner, this);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Shinbi_RMSkill_DamageEffect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	UE_LOG(LogTemp, Display, TEXT("Apply Damage4"));

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		1.5f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

