#include "GAS/GE_ExecutionCalculation.h"

#include "AbilitySystemComponent.h"
#include "WAttributeSet.h"
#include "Character/WCharacterBase.h"

struct FDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackStat);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DefenseStat);

	FDamageStatics()
	{
		// 공격자(Source)의 공격력 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWAttributeSet, AttackStat, Source, false);
		// 피격자(Target)의 방어력 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWAttributeSet, DefenseStat, Target, false);
	}
};

static const FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UGE_ExecutionCalculation::UGE_ExecutionCalculation()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackStatDef);
	RelevantAttributesToCapture.Add(DamageStatics().DefenseStatDef);
}

void UGE_ExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceAvatar = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	SetAttacker(ExecutionParams);

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float AttackStat = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackStatDef, EvaluationParameters, AttackStat);

	float DefenseStat = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseStatDef, EvaluationParameters, DefenseStat);

	FGameplayTag CoefficientTag = FGameplayTag::RequestGameplayTag(FName("ability.data.damage"));
	float SkillCoefficient = Spec.GetSetByCallerMagnitude(CoefficientTag, false, 1.0f);

	// 3. 데미지 계산식 수행 (예: (공격력 * 계수) - 방어력, 최소 1보장)
	float RawDamage = (AttackStat * SkillCoefficient) - DefenseStat;
	float FinalDamage = FMath::Max(1.f, RawDamage);

	// 4. 최종 데미지 전달
	if (FinalDamage > 0.f)
	{
		FGameplayModifierEvaluatedData EvaluatedData(
			UWAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive, 
			-FinalDamage
		);
		UE_LOG(LogTemp, Warning, TEXT("%f"), FinalDamage);
		OutExecutionOutput.AddOutputModifier(EvaluatedData);
	}
}

void UGE_ExecutionCalculation::SetAttacker(const FGameplayEffectCustomExecutionParameters& ExecutionParams) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	AActor* InstigatorActor = Spec.GetEffectContext().GetInstigator();
	if (InstigatorActor)
	{
		AWCharacterBase* AttackerChar = Cast<AWCharacterBase>(InstigatorActor);
		if (AttackerChar)
		{
			if (AAOSCharacter* Target = Cast<AAOSCharacter>(ExecutionParams.GetTargetAbilitySystemComponent()->GetAvatarActor()))
			{
				Target->RegisterAttacker(AttackerChar->GetPlayerState<AGamePlayerState>());
			}
		}
	}
}
