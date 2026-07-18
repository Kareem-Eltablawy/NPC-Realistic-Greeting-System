#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"
#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "Character/NPC/YoungWoman/YoungWomanFaceAnimActorComponent.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"

UYoungWomanActorComponent::UYoungWomanActorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UYoungWomanActorComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerYW = Cast<AYoungWoman>(GetOwner());
    if (!OwnerYW) { return; }

    DataAsset = OwnerYW->AnimDataAsset;
    FaceAC    = OwnerYW->FindComponentByClass<UYoungWomanFaceAnimActorComponent>();

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanBrain] '%s': BeginPlay -- DA=%s  FaceAC=%s"),
        *GetOwner()->GetName(),
        DataAsset ? *DataAsset->GetName() : TEXT("NULL"),
        FaceAC    ? TEXT("OK")            : TEXT("NULL"));
}

// =============================================================================
// Idle
// =============================================================================
void UYoungWomanActorComponent::NotifyIdleLooped()
{
    switch (CurrentIdleState)
    {
    case EYoungWomanIdleState::Idle_A: CurrentIdleState = EYoungWomanIdleState::Idle_B; break;
    case EYoungWomanIdleState::Idle_B: CurrentIdleState = EYoungWomanIdleState::Idle_C; break;
    case EYoungWomanIdleState::Idle_C: CurrentIdleState = EYoungWomanIdleState::Idle_A; break;
    }
}

// =============================================================================
// Greeting flow
// =============================================================================
void UYoungWomanActorComponent::OnSarahGreeted(const FVector& SarahWorldLocation)
{
    if (CurrentGreetingState != EYoungWomanGreetingState::Idle) { return; }

    UWorld* World = GetWorld();
    if (!World) { return; }

    // ① Position LookingPlace toward Sarah immediately (alpha stays 0 until delay fires).
    if (OwnerYW)
    {
        OwnerYW->MoveLookingPlaceTo(SarahWorldLocation);
    }

    // ② Start hospitality delay.
    float MinDelay = 0.5f, MaxDelay = 1.0f;
    if (DataAsset)
    {
        switch (DataAsset->Hospitality)
        {
        case EYoungWomanHospitality::Friendly:
            MinDelay = 0.5f; MaxDelay = 1.0f; break;
        case EYoungWomanHospitality::Slower:
            MinDelay = 1.0f; MaxDelay = 2.0f; break;
        case EYoungWomanHospitality::LessFriendly:
            MinDelay = 2.0f; MaxDelay = 3.0f; break;
        }
    }

    // Discrete 0.1 s steps for a consistent feel.
    const int32 StepMin = FMath::RoundToInt(MinDelay * 10.f);
    const int32 StepMax = FMath::RoundToInt(MaxDelay * 10.f);
    const float Delay   = FMath::RandRange(StepMin, StepMax) / 10.f;

    CurrentGreetingState = EYoungWomanGreetingState::DelayPending;

    World->GetTimerManager().ClearTimer(ReplyDelayTimer);
    World->GetTimerManager().SetTimer(
        ReplyDelayTimer,
        this,
        &UYoungWomanActorComponent::OnReplyDelayFired,
        Delay,
        /*bLoop=*/false);

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanBrain] '%s': Sarah greeted -- LookingPlace set, reply in %.1f s."),
        *GetOwner()->GetName(), Delay);
}

void UYoungWomanActorComponent::OnSarahLeft()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReplyDelayTimer);
    }

    if (CurrentGreetingState == EYoungWomanGreetingState::DelayPending)
    {
        // Greeting never fired — undo LookingPlace positioning.
        if (OwnerYW) { OwnerYW->StopLookAt(); }
        CurrentGreetingState = EYoungWomanGreetingState::Idle;
    }

    // If state == Greeting the face anim is playing; let it finish naturally.
    // NotifyGreetingEnded() will call StopLookAt() and reset to Idle.

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanBrain] '%s': Sarah left -- timer cancelled."),
        *GetOwner()->GetName());
}

void UYoungWomanActorComponent::OnReplyDelayFired()
{
    TriggerGreeting();
}

void UYoungWomanActorComponent::TriggerGreeting()
{
    if (!DataAsset || DataAsset->Greetings.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[YoungWomanBrain] '%s': Greetings array empty -- skipped."),
            *GetOwner()->GetName());
        CurrentGreetingState = EYoungWomanGreetingState::Idle;
        if (OwnerYW) { OwnerYW->StopLookAt(); }
        return;
    }

    const int32 Index    = FMath::RandRange(0, DataAsset->Greetings.Num() - 1);
    CurrentGreetingState = EYoungWomanGreetingState::Greeting;

    // ③ Enable head look-at (LookingPlace was already positioned in ①).
    if (OwnerYW) { OwnerYW->StartLookAt(); }

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanBrain] '%s': greeting index=%d, LookAt ON."),
        *GetOwner()->GetName(), Index);

    if (FaceAC)
    {
        FaceAC->PlayGreeting(Index);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[YoungWomanBrain] '%s': no FaceAC -- skipped."),
            *GetOwner()->GetName());
        CurrentGreetingState = EYoungWomanGreetingState::Idle;
        if (OwnerYW) { OwnerYW->StopLookAt(); }
    }
}

void UYoungWomanActorComponent::NotifyGreetingEnded()
{
    // ④ Disable look-at and reset LookingPlace.
    if (OwnerYW) { OwnerYW->StopLookAt(); }

    CurrentGreetingState = EYoungWomanGreetingState::Idle;

    UE_LOG(LogTemp, Log,
        TEXT("[YoungWomanBrain] '%s': greeting ended -- LookAt OFF, Idle."),
        *GetOwner()->GetName());
}
