#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/NPCActorComponent.h"
#include "YoungWomanActorComponent.generated.h"

class AYoungWoman;
class UYoungWomanAnimationDataAsset;
class UYoungWomanFaceAnimActorComponent;

UENUM(BlueprintType)
enum class EYoungWomanIdleState : uint8
{
    Idle_A UMETA(DisplayName = "Idle A"),
    Idle_B UMETA(DisplayName = "Idle B"),
    Idle_C UMETA(DisplayName = "Idle C"),
};

UENUM(BlueprintType)
enum class EYoungWomanGreetingState : uint8
{
    Idle         UMETA(DisplayName = "Idle -- waiting"),
    DelayPending UMETA(DisplayName = "Delay Pending -- timer running"),
    Greeting     UMETA(DisplayName = "Greeting -- face anim playing"),
};

/**
 * UYoungWomanActorComponent — body brain for every YoungWoman NPC.
 *
 * Greeting flow (G-press driven, fully event-based):
 *
 *   1. ASarahBase::TryInteract()
 *         -> Brain->OnSarahGreeted(SarahWorldLocation)
 *             -> OwnerYW->MoveLookingPlaceTo(loc)    [SC positioned; alpha=0]
 *             -> starts hospitality-delay timer
 *
 *   2. Timer fires -> TriggerGreeting()
 *         -> picks random Greetings index
 *         -> OwnerYW->StartLookAt()                  [alpha -> 1]
 *         -> FaceAC->PlayGreeting(index)
 *
 *   3. FaceAC finishes -> Brain->NotifyGreetingEnded()
 *         -> OwnerYW->StopLookAt()                   [alpha -> 0; SC reset]
 *         -> state -> Idle
 *
 * Zero tick. No polling. Every transition is an explicit function call.
 */
UCLASS(ClassGroup = (YoungWoman), meta = (BlueprintSpawnableComponent))
class GAME_API UYoungWomanActorComponent : public UNPCActorComponent
{
    GENERATED_BODY()

public:
    UYoungWomanActorComponent();

    virtual void BeginPlay() override;

    // ── Brain state ───────────────────────────────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YoungWoman|Brain")
    EYoungWomanIdleState CurrentIdleState = EYoungWomanIdleState::Idle_A;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YoungWoman|Brain")
    EYoungWomanGreetingState CurrentGreetingState = EYoungWomanGreetingState::Idle;

    // ── Public API ────────────────────────────────────────────────────────────

    /** Advances idle state A -> B -> C -> A. Called by the body anim instance. */
    void NotifyIdleLooped();

    /**
     * Called by ASarahBase when Sarah presses G.
     * Positions LookingPlace immediately (head alpha stays 0) then starts the
     * hospitality-delay timer.  No-op if already greeting or delay pending.
     */
    void OnSarahGreeted(const FVector& SarahWorldLocation);

    /** Called by ASarahBase on overlap exit. */
    void OnSarahLeft();

    /** Called by UYoungWomanFaceAnimActorComponent when face anim + hold end. */
    void NotifyGreetingEnded();

    // ── Cached data asset ─────────────────────────────────────────────────────

    UPROPERTY()
    TObjectPtr<const UYoungWomanAnimationDataAsset> DataAsset;

private:
    UPROPERTY()
    TObjectPtr<AYoungWoman> OwnerYW;

    UPROPERTY()
    TObjectPtr<UYoungWomanFaceAnimActorComponent> FaceAC;

    FTimerHandle ReplyDelayTimer;

    void OnReplyDelayFired();
    void TriggerGreeting();
};
