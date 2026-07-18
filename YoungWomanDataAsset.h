#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YoungWomanDataAsset.generated.h"

UENUM(BlueprintType)
enum class EYoungWomanPersonality : uint8
{
    Friendly    UMETA(DisplayName = "Friendly"),
    Normal      UMETA(DisplayName = "Normal"),
    Ignoring    UMETA(DisplayName = "Ignoring"),
    Rude        UMETA(DisplayName = "Rude"),
    Antagonize  UMETA(DisplayName = "Antagonize"),
};

/**
 * UYoungWomanDataAsset
 *
 * Per-NPC personality configuration.
 * Assign to any AYoungWoman via the Details panel.
 */
UCLASS(BlueprintType)
class GAME_API UYoungWomanDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    /**
     * Defines how this NPC reacts to the player and conversation triggers.
     *
     *  Friendly   – may initiate greetings; replies positively.
     *  Normal     – replies when addressed; no strong bias either way.
     *  Ignoring   – never initiates; does not respond to interaction.
     *  Rude       – refuses to reply; may play a dismissive reaction.
     *  Antagonize – replies with hostile or insulting dialogue.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    EYoungWomanPersonality Personality = EYoungWomanPersonality::Normal;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("YoungWomanDataAsset", GetFName());
    }
};
