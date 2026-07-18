#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/YoungWoman/YoungWomanAnimInstance.h"
#include "NPCDamselAnimInstance.generated.h"

class ANPCDamsel;

// =============================================================================
// FNPCDamselIdleProxy
// Inherits FYoungWomanAnimProxy to gain ApplyLookAt().
// =============================================================================
/**
 * Three-idle crossfade proxy for the NPCDamsel's body.
 *
 * Evaluate:
 *   1. Generates the idle crossfade pose (A / B / C + blend-weight transition).
 *   2. Calls FYoungWomanAnimProxy::ApplyLookAt() — inherited — to layer the head
 *      look-at rotation on top when LookAtAlpha > 0.
 *
 * LookAt data (LookAtTargetWorld, LookAtAlpha, CachedHeadBoneIndex) is inherited
 * from FYoungWomanAnimProxy and synced by UYoungWomanAnimInstance::NativeUpdateAnimation
 * via the MyProxy pointer set in CreateAnimInstanceProxy().
 */
struct FNPCDamselIdleProxy : public FYoungWomanAnimProxy
{
public:
    FNPCDamselIdleProxy() = default;
    explicit FNPCDamselIdleProxy(UAnimInstance* InInstance)
        : FYoungWomanAnimProxy(InInstance) {}

    UAnimSequence* IdleA = nullptr;
    UAnimSequence* IdleB = nullptr;
    UAnimSequence* IdleC = nullptr;

    int32          CurrentIdleIndex = 0;
    UAnimSequence* ActiveSequence   = nullptr;
    UAnimSequence* NextSequence     = nullptr;
    float          ActiveTime       = 0.f;
    float          NextTime         = 0.f;
    float          BlendWeight      = 0.f;
    bool           bInTransition    = false;

    virtual bool Evaluate(FPoseContext& Output) override;
};

// =============================================================================
// UNPCDamselAnimInstance
// =============================================================================
UCLASS()
class GAME_API UNPCDamselAnimInstance : public UYoungWomanAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy()                             override;
    virtual void                DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
    FNPCDamselIdleProxy*   DamselProxy = nullptr;
    TObjectPtr<ANPCDamsel> DamselChar  = nullptr;

    float BlendInterpSpeed = 4.f;

    UAnimSequence* GetIdleForIndex(int32 Index) const;
    void           BeginBlendToNextIdle();
};
