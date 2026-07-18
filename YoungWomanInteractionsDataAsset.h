#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"
#include "YoungWomanInteractionsDataAsset.generated.h"

/**
 * Hospitality level for a YoungWoman NPC.
 * Friendly     – warm, welcoming  (range 1-2 s)
 * Slower       – neutral / calm   (range 2-3 s)
 * LessFriendly – cold / guarded   (range 3-4 s)
 *
 * NOTE: prefer UYoungWomanAnimationDataAsset::Hospitality (EYoungWomanHospitality)
 * for new NPCs.  This asset is kept for legacy interaction-only setups that do
 * not use the full YoungWoman anim pipeline.
 */
UENUM(BlueprintType)
enum class EYoungWomanHospitalityLevel : uint8
{
    Friendly        UMETA(DisplayName = "Friendly (1-2 s)"),
    Slower          UMETA(DisplayName = "Slower (2-3 s)"),
    LessFriendly    UMETA(DisplayName = "Less Friendly (3-4 s)"),
};

/**
 * UYoungWomanInteractionsDataAsset
 *
 * Lightweight interaction data for YoungWoman NPCs.
 * GreetingAnimations[i] pairs with GreetingAudios[i].
 */
UCLASS(BlueprintType)
class GAME_API UYoungWomanInteractionsDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
    TArray<TSoftObjectPtr<UAnimSequence>> GreetingAnimations;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactions")
    TArray<TSoftObjectPtr<USoundBase>> GreetingAudios;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hospitality")
    EYoungWomanHospitalityLevel Hospitality = EYoungWomanHospitalityLevel::Friendly;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("YoungWomanInteractionsDataAsset", GetFName());
    }
};
