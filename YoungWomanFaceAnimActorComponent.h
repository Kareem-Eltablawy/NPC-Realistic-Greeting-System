#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/NPCActorComponent.h"
#include "YoungWomanFaceAnimActorComponent.generated.h"

class AYoungWoman;
class UYoungWomanActorComponent;
class UYoungWomanFaceAnim;
class UAudioComponent;
class UAnimSequence;
class USoundBase;

/**
 * Face-animation brain for every YoungWoman NPC.
 *
 * Owns:
 *   • Hard-loading UAnimSequence and USoundBase from the Greetings array
 *     in UYoungWomanAnimationDataAsset (done once in BeginPlay).
 *   • Calling UYoungWomanFaceAnim::PlayGreetingFromAC when the body brain fires.
 *   • Spawning the paired audio clip.
 *   • Calling UYoungWomanActorComponent::NotifyGreetingEnded when the face
 *     anim instance reports that the anim + hold delay finished.
 *
 * The face anim instance is a pure proxy reader — zero logic lives there.
 */
UCLASS(ClassGroup = (YoungWoman), meta = (BlueprintSpawnableComponent))
class GAME_API UYoungWomanFaceAnimActorComponent : public UNPCActorComponent
{
    GENERATED_BODY()

public:
    UYoungWomanFaceAnimActorComponent();

    virtual void BeginPlay() override;

    /** Called by UYoungWomanActorComponent after the delay fires. */
    void PlayGreeting(int32 Index);

    /** Called back by UYoungWomanFaceAnim when anim + hold delay finish. */
    void NotifyFaceAnimEnded();

private:
    UPROPERTY()
    TObjectPtr<AYoungWoman> OwnerYW;

    UPROPERTY()
    TObjectPtr<UYoungWomanActorComponent> BodyBrain;

    UPROPERTY()
    TObjectPtr<UYoungWomanFaceAnim> FaceAnimInstance;

    // Hard-loaded from Greetings[i].Animation and Greetings[i].Audio
    TArray<TObjectPtr<UAnimSequence>> LoadedAnims;
    TArray<TObjectPtr<USoundBase>>    LoadedAudio;

    UPROPERTY()
    TObjectPtr<UAudioComponent> ActiveAudio;

    void StopAudioIfPlaying();
};
