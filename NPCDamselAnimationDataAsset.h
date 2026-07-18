#pragma once

#include "CoreMinimal.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "NPCDamselAnimationDataAsset.generated.h"

UCLASS(BlueprintType)
class GAME_API UNPCDamselAnimationDataAsset : public UYoungWomanAnimationDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPCDamsel|Body|Idle")
    TObjectPtr<UAnimSequence> IdleA;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPCDamsel|Body|Idle")
    TObjectPtr<UAnimSequence> IdleB;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPCDamsel|Body|Idle")
    TObjectPtr<UAnimSequence> IdleC;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPCDamsel|Body|Idle",
        meta = (ClampMin = "0.5", ClampMax = "30.0"))
    float IdleBlendInterpSpeed = 4.0f;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("NPCDamselAnimationDataAsset", GetFName());
    }
};
