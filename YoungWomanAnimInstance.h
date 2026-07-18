#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "YoungWomanAnimInstance.generated.h"

class AYoungWoman;
class UCharacterMovementComponent;
class UYoungWomanActorComponent;
class FProperty;

// =============================================================================
// FYoungWomanAnimProxy — worker-thread proxy for the body mesh anim instance.
// =============================================================================
struct FYoungWomanAnimProxy : public FAnimInstanceProxy
{
public:
    FYoungWomanAnimProxy() = default;
    explicit FYoungWomanAnimProxy(UAnimInstance* InInstance)
        : FAnimInstanceProxy(InInstance) {}

    // ── Game-thread -> worker-thread data ─────────────────────────────────────
    FVector             LookAtTargetWorld            = FVector::ZeroVector;
    float               LookAtAlpha                  = 0.f;
    int32               CachedHeadBoneIndex          = INDEX_NONE;
    EYoungWomanBoneAxis CachedLookAtAxis             = EYoungWomanBoneAxis::Y;
    FVector             SmoothedLookDirCS            = FVector::ZeroVector;
    float               DeltaTimeForLookAt           = 0.f;
    float               LookAtTrackingSpeedRadPerSec = (120.f * PI / 180.f);

    virtual bool Evaluate(FPoseContext& Output) override;

protected:
    void ApplyLookAt(FPoseContext& Output);
};

// =============================================================================
// UYoungWomanAnimInstance — game-thread anim instance for AYoungWoman body mesh.
// =============================================================================
UCLASS()
class GAME_API UYoungWomanAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UYoungWomanAnimInstance();

    // ── Body locomotion ────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, Category = "YoungWoman|Locomotion")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "YoungWoman|Locomotion")
    bool bIsMoving = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YoungWoman|Animation|LookAt")
    bool bSuppressSharedPostProcessLookAt = true;

protected:
    virtual void NativeInitializeAnimation()              override;
    virtual void NativeUpdateAnimation(float DeltaTime)   override;
    virtual void NativePostEvaluateAnimation()            override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

    UPROPERTY(Transient) TObjectPtr<AYoungWoman>                 YoungWoman = nullptr;
    UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> MoveComp   = nullptr;
    UPROPERTY(Transient) TObjectPtr<UYoungWomanActorComponent>   Brain      = nullptr;

    FYoungWomanAnimProxy* MyProxy = nullptr;

    float CurrentLookAtAlpha = 0.f;

private:
    void SuppressSharedPostProcessHeadMovementControl();

    FProperty*                    CachedHeadMovementControlProp     = nullptr;
    TWeakObjectPtr<UAnimInstance> CachedPostProcessInstanceForProp;
};
