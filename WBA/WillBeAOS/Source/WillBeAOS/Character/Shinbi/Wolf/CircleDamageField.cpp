#include "Character/Shinbi/Wolf/CircleDamageField.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SphereComponent.h"
#include "Gimmick/Nexus.h"
#include "Gimmick/Tower.h"
#include "Interface/GetInfoInterface.h"


ACircleDamageField::ACircleDamageField()
{
	DamageCollision = CreateDefaultSubobject<USphereComponent>("DamageCollision");
	DamageCollision->SetupAttachment(RootComponent);
	DamageCollision->SetSphereRadius(FieldRadius);
	DamageCollision->SetGenerateOverlapEvents(true);
	
	PrimaryActorTick.bCanEverTick = true;
}

void ACircleDamageField::InitField(AActor* InOwner, const float InRadius, const float InLifeTime)
{
	FieldOwner = InOwner;
	FieldRadius = InRadius;
	LifeTime = InLifeTime;

	Player = Cast<AAOSCharacter>(InOwner);
	DamageCollision->SetSphereRadius(FieldRadius);
	SetLifeSpan(InLifeTime);

	if (Player->GetTeamID() == E_TeamID::Blue)
	{
		DamageCollision->SetCollisionObjectType(TeamCollision::BlueTeam);
		DamageCollision->SetCollisionResponseToChannel(TeamCollision::BlueTeam, ECR_Ignore);
		DamageCollision->SetCollisionResponseToChannel(TeamCollision::RedTeam, ECR_Overlap);
	}
	else
	{
		DamageCollision->SetCollisionObjectType(TeamCollision::RedTeam);
		DamageCollision->SetCollisionResponseToChannel(TeamCollision::RedTeam, ECR_Ignore);
		DamageCollision->SetCollisionResponseToChannel(TeamCollision::BlueTeam, ECR_Overlap);
	}
	
	TArray<AActor*> OverlappingActors;
	DamageCollision->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		StartDamageToActor(Actor);
	}
}

void ACircleDamageField::BeginPlay()
{
	Super::BeginPlay();
}

void ACircleDamageField::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACircleDamageField::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!HasAuthority()) return;
	StartDamageToActor(OtherActor);
}

void ACircleDamageField::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (!HasAuthority()) return;

	if (FTimerHandle* Timer = DamageTimers.Find(OtherActor))
	{
		GetWorldTimerManager().ClearTimer(*Timer);
		DamageTimers.Remove(OtherActor);
	}
}

void ACircleDamageField::StartDamageToActor(AActor* HitActor)
{
	if (FieldOwner == HitActor) return;
	if (this ==  HitActor) return;

	const IGetInfoInterface* TargetTeam = Cast<IGetInfoInterface>(HitActor);
	const IGetInfoInterface* SourceTeam = Cast<IGetInfoInterface>(FieldOwner);

	if (!TargetTeam || !SourceTeam) return;

	if (DamageTimers.Contains(HitActor)) return;

	DamageToEnemy(HitActor);

	FTimerHandle NewTimer;
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &ThisClass::DamageToEnemy, HitActor);
	GetWorldTimerManager().SetTimer(NewTimer, Delegate, 0.5f, true);
	DamageTimers.Add(HitActor, NewTimer);
}

void ACircleDamageField::DamageToEnemy(AActor* HitActor)
{	
	if (!HitActor) return;

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(FieldOwner);
	if (!SourceASC) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FVector HitLocation = GetHitLocationForActor(HitActor);

	FHitResult HitResult;
	HitResult.Location = HitLocation;
	HitResult.ImpactPoint = HitLocation;
	HitResult.HitObjectHandle = FActorInstanceHandle(HitActor);

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(FieldOwner, this);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Shinbi_ESkill_DamageEffect, 1.f, EffectContext);
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag("ability.data.damage"),
		0.5f
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

FVector ACircleDamageField::GetHitLocationForActor(AActor* HitActor) const
{
	FVector FieldCenter = GetActorLocation();
	FVector TargetLocation = HitActor->GetActorLocation();

	FVector ClosestPoint;
	UPrimitiveComponent* TargetComp;

	if (ANexus* Nexus = Cast<ANexus>(HitActor))
		TargetComp = Nexus->GetMesh();
	else if (ATower* Tower = Cast<ATower>(HitActor))
		TargetComp = Tower->GetMesh();
	else
		TargetComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
	
	if (TargetComp)
	{
		TargetComp->GetClosestPointOnCollision(FieldCenter, ClosestPoint);
		return ClosestPoint;
	}

	return TargetLocation;
} 