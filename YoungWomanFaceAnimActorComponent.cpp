#include "Character/NPC/YoungWoman/YoungWomanFaceAnimActorComponent.h"
#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"
#include "Character/NPC/YoungWoman/YoungWomanFaceAnim.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

UYoungWomanFaceAnimActorComponent::UYoungWomanFaceAnimActorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// =============================================================================
// BeginPlay — hard-loads every Greetings entry once so PlayGreeting is zero-latency.
// =============================================================================
void UYoungWomanFaceAnimActorComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerYW = Cast<AYoungWoman>(GetOwner());
    if (!OwnerYW) { return; }

    BodyBrain = OwnerYW->FindComponentByClass<UYoungWomanActorComponent>();

    const UYoungWomanAnimationDataAsset* DA = OwnerYW->AnimDataAsset;
    if (!DA)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[YoungWomanFaceAC] '%s': No AnimDataAsset — greetings disabled."),
            *GetOwner()->GetName());
        return;
    }

    LoadedAnims.Reset();
    LoadedAudio.Reset();

    for (const FYoungWomanGreeting& Entry : DA->Greetings)
    {
        UAnimSequence* Anim  = Entry.Animation.LoadSynchronous();
        USoundBase*    Sound = Entry.Audio.LoadSynchronous();

        LoadedAnims.Add(Anim);   // null preserved — index stays aligned
        LoadedAudio.Add(Sound);

        if (!Anim)
            UE_LOG(LogTemp, Warning,
                TEXT("[YoungWomanFaceAC] '%s': Greetings entry has null Animation."),
                *GetOwner()->GetName());
    }

    if (USkeletalMeshComponent* FaceMesh = OwnerYW->GetFaceMesh())
    {
        FaceAnimInstance = Cast<UYoungWomanFaceAnim>(FaceMesh->GetAnimInstance());
        if (!FaceAnimInstance)
            UE_LOG(LogTemp, Warning,
                TEXT("[YoungWomanFaceAC] '%s': FaceMesh has no UYoungWomanFaceAnim."),
                *GetOwner()->GetName());
    }

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanFaceAC] '%s': Ready — %d greeting entries."),
        *GetOwner()->GetName(), LoadedAnims.Num());
}

// =============================================================================
// PlayGreeting
// =============================================================================
void UYoungWomanFaceAnimActorComponent::PlayGreeting(int32 Index)
{
    if (!FaceAnimInstance)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[YoungWomanFaceAC] '%s': PlayGreeting(%d) — no FaceAnimInstance."),
            *GetOwner()->GetName(), Index);
        NotifyFaceAnimEnded();
        return;
    }

    if (!LoadedAnims.IsValidIndex(Index) || !LoadedAnims[Index])
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[YoungWomanFaceAC] '%s': PlayGreeting(%d) — null anim at index."),
            *GetOwner()->GetName(), Index);
        NotifyFaceAnimEnded();
        return;
    }

    FaceAnimInstance->PlayGreetingFromAC(Index, LoadedAnims[Index]);

    StopAudioIfPlaying();
    if (LoadedAudio.IsValidIndex(Index) && LoadedAudio[Index] && OwnerYW)
    {
        ActiveAudio = UGameplayStatics::SpawnSoundAttached(
            LoadedAudio[Index],
            OwnerYW->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            /*bStopWhenAttachedToDestroyed=*/true);
    }

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanFaceAC] '%s': PlayGreeting(%d) -> '%s'."),
        *GetOwner()->GetName(), Index, *LoadedAnims[Index]->GetName());
}

// =============================================================================
// NotifyFaceAnimEnded
// =============================================================================
void UYoungWomanFaceAnimActorComponent::NotifyFaceAnimEnded()
{
    StopAudioIfPlaying();
    if (BodyBrain) { BodyBrain->NotifyGreetingEnded(); }
}

// =============================================================================
// StopAudioIfPlaying
// =============================================================================
void UYoungWomanFaceAnimActorComponent::StopAudioIfPlaying()
{
    if (!ActiveAudio) { return; }
    if (ActiveAudio->IsPlaying()) { ActiveAudio->Stop(); }
    ActiveAudio = nullptr;
}
