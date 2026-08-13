#include "GAS/Cue/WBlueprintFunctionLibrary.h"

#include "Character/Wraith/Projectile/FBulletTargetData.h"


bool UWBlueprintFunctionLibrary::GetBulletContextData(const FGameplayCueParameters& CueParameters, FVector& MuzzleLoc,
                                                      FVector& StrikePoint, float& Speed)
{
	const FBulletContext* Context = static_cast<const FBulletContext*>(CueParameters.EffectContext.Get());
	if (!Context) return false;

	MuzzleLoc = Context->MuzzleLoc;
	StrikePoint = Context->StrikePoint;
	Speed = Context->Speed;
	
	return true;
}
