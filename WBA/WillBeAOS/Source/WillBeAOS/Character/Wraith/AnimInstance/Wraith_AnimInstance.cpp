#include "Character/Wraith/AnimInstance/Wraith_AnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"


void UWraith_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UWraith_AnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	AActor* OwningActor = GetOwningActor();
	if (!OwningActor) return;

	CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor);
	if (CachedASC)
	{
		FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("state.combat"));
		FGameplayTag SnipeTag  = FGameplayTag::RequestGameplayTag(FName("state.wraith.shootingmode.snipe"));
		FGameplayTag BombTag   = FGameplayTag::RequestGameplayTag(FName("state.wraith.shootingmode.bomb"));

		CachedASC->RegisterGameplayTagEvent(CombatTag, EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(this, &UWraith_AnimInstance::OnShootingModeTagCountChanged);
    
		CachedASC->RegisterGameplayTagEvent(SnipeTag, EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(this, &UWraith_AnimInstance::OnShootingModeTagCountChanged);

		CachedASC->RegisterGameplayTagEvent(BombTag, EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(this, &UWraith_AnimInstance::OnShootingModeTagCountChanged);

		OnShootingModeTagCountChanged(FGameplayTag(), 0);
	}
}

UBlendSpace* UWraith_AnimInstance::SelectAimOffsetByShootingTag(FGameplayTag ShootingTag) const
{
	static const FGameplayTag SnipeTag  = FGameplayTag::RequestGameplayTag(FName("state.wraith.shootingmode.snipe"));
	static const FGameplayTag BombTag   = FGameplayTag::RequestGameplayTag(FName("state.wraith.shootingmode.bomb"));
	static const FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("state.combat"));

	if (ShootingTag.MatchesTagExact(SnipeTag))  return AO_Snipe;
	if (ShootingTag.MatchesTagExact(BombTag))   return AO_Bomb;
	if (ShootingTag.MatchesTagExact(CombatTag)) return AO_Combat;

	return AO_NonCombat;
}

void UWraith_AnimInstance::OnShootingModeTagCountChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!CachedASC) return;

	FGameplayTagContainer OwnedTags;
	CachedASC->GetOwnedGameplayTags(OwnedTags);

	FGameplayTag FinalModeTag;

	FGameplayTag QSkillTag = FGameplayTag::RequestGameplayTag("state.wraith.shootingmode.snipe");
	FGameplayTag ESkillTag = FGameplayTag::RequestGameplayTag("state.wraith.shootingmode.bomb");

	if (OwnedTags.HasTagExact(QSkillTag))
	{
		FinalModeTag = QSkillTag;
	}
	else if (OwnedTags.HasTagExact(ESkillTag))
	{
		FinalModeTag = ESkillTag;
	}
	else if (OwnedTags.HasTagExact(FGameplayTag::RequestGameplayTag("state.combat")))
	{
		FinalModeTag = FGameplayTag::RequestGameplayTag("state.combat");
	}
	else
	{
		FinalModeTag = FGameplayTag::RequestGameplayTag("state.noncombat");
	}

	if (CurrentShootingModeTag != FinalModeTag)
	{
		CurrentShootingModeTag = FinalModeTag;
	}
}
