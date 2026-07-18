#include "Character/NPC/YoungWoman/NPCDamsel/NPCDamselAnimInstance.h"
#include "Character/NPC/YoungWoman/NPCDamsel/NPCDamsel.h"
#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/NPCDamselAnimationDataAsset.h"
#include "AnimationRuntime.h"
#include "BonePose.h"

// =============================================================================
// FNPCDamselIdleProxy::Evaluate
// =============================================================================
bool FNPCDamselIdleProxy::Evaluate(FPoseContext& Output)
{
    Output.ResetToRefPose();
    if (!ActiveSequence) { return false; }

    const FBoneContainer& BC = Output.Pose.GetBoneContainer();

    // ── Step 1: generate idle crossfade pose ──────────────────────────────────
    FCompactPose AP; FBlendedCurve AC; UE::Anim::FStackAttributeContainer AA;
    AP.SetBoneContainer(&BC); AC.InitFrom(BC);
    FAnimationPoseData AD(AP, AC, AA);
    ActiveSequence->GetAnimationPose(
        AD, FAnimExtractContext((double)ActiveTime, false));

    if (!NextSequence || BlendWeight <= KINDA_SMALL_NUMBER)
    {
        Output.Pose  = AP;
        Output.Curve = AC;
    }
    else
    {
        FCompactPose NP; FBlendedCurve NC; UE::Anim::FStackAttributeContainer NA;
        NP.SetBoneContainer(&BC); NC.InitFrom(BC);
        FAnimationPoseData ND(NP, NC, NA);
        NextSequence->GetAnimationPose(
            ND, FAnimExtractContext((double)NextTime, false));

        UE::Anim::FStackAttributeContainer OA;
        FAnimationPoseData OD(Output.Pose, Output.Curve, OA);
        FAnimationRuntime::BlendTwoPosesTogether(AD, ND, 1.0f - BlendWeight, OD);
    }

    // ── Step 2: layer head look-at on top ─────────────────────────────────────
    // ApplyLookAt() is defined in FYoungWomanAnimProxy (parent).
    // LookAtAlpha and LookAtTargetWorld are synced by
    // UYoungWomanAnimInstance::NativeUpdateAnimation via the MyProxy upcast.
    if (LookAtAlpha > KINDA_SMALL_NUMBER && CachedHeadBoneIndex != INDEX_NONE)
    {
        ApplyLookAt(Output);
    }

    return true;
}

// =============================================================================
// Proxy lifetime
// CreateAnimInstanceProxy also sets MyProxy (base-class ptr) so that
// UYoungWomanAnimInstance::NativeUpdateAnimation syncs look-at data into the
// same FNPCDamselIdleProxy object through the typed FYoungWomanAnimProxy* pointer.
// =============================================================================
FAnimInstanceProxy* UNPCDamselAnimInstance::CreateAnimInstanceProxy()
{
    DamselProxy = new FNPCDamselIdleProxy(this);
    MyProxy     = DamselProxy;   // upcast: FNPCDamselIdleProxy* → FYoungWomanAnimProxy*
    return DamselProxy;
}

void UNPCDamselAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
    DamselProxy = nullptr;
    MyProxy     = nullptr;      // null both typed aliases before delete
    delete InProxy;
}

// =============================================================================
// GetIdleForIndex
// =============================================================================
UAnimSequence* UNPCDamselAnimInstance::GetIdleForIndex(int32 Index) const
{
    if (!DamselProxy) { return nullptr; }
    switch (Index)
    {
    case 0:  return DamselProxy->IdleA;
    case 1:  return DamselProxy->IdleB;
    default: return DamselProxy->IdleC;
    }
}

// =============================================================================
// NativeInitializeAnimation
// =============================================================================
void UNPCDamselAnimInstance::NativeInitializeAnimation()
{
    // Base: caches YoungWoman, MoveComp, Brain, head bone index into proxy.
    Super::NativeInitializeAnimation();

    DamselChar = Cast<ANPCDamsel>(YoungWoman);
    if (!DamselChar || !DamselProxy) { return; }

    const UNPCDamselAnimationDataAsset* DA =
        Cast<UNPCDamselAnimationDataAsset>(DamselChar->AnimDataAsset);

    if (!DA)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[NPCDamselAnim] '%s': AnimDataAsset is not UNPCDamselAnimationDataAsset."),
            *DamselChar->GetName());
        return;
    }

    BlendInterpSpeed = DA->IdleBlendInterpSpeed;

    DamselProxy->IdleA = DA->IdleA;
    DamselProxy->IdleB = DA->IdleB;
    DamselProxy->IdleC = DA->IdleC;

    if (!DamselProxy->IdleA)
        UE_LOG(LogTemp, Warning, TEXT("[NPCDamselAnim] '%s': IdleA NULL."), *DamselChar->GetName());
    if (!DamselProxy->IdleB)
        UE_LOG(LogTemp, Warning, TEXT("[NPCDamselAnim] '%s': IdleB NULL."), *DamselChar->GetName());
    if (!DamselProxy->IdleC)
        UE_LOG(LogTemp, Warning, TEXT("[NPCDamselAnim] '%s': IdleC NULL."), *DamselChar->GetName());

    DamselProxy->CurrentIdleIndex = 0;
    DamselProxy->ActiveSequence   = DamselProxy->IdleA;
    DamselProxy->ActiveTime       = 0.f;
    DamselProxy->NextSequence     = nullptr;
    DamselProxy->BlendWeight      = 0.f;
    DamselProxy->bInTransition    = false;
}

// =============================================================================
// NativeUpdateAnimation
// =============================================================================
void UNPCDamselAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    // Base: updates Speed, bIsMoving, and — critically — syncs
    // LookAtAlpha / LookAtTargetWorld / CachedHeadBoneIndex into MyProxy
    // (which IS DamselProxy due to the upcast in CreateAnimInstanceProxy).
    Super::NativeUpdateAnimation(DeltaTime);

    if (!DamselProxy || !DamselChar) { return; }

    // ── Tick crossfade ────────────────────────────────────────────────────────
    if (DamselProxy->bInTransition)
    {
        if (DamselProxy->NextSequence)
        {
            const float NLen = DamselProxy->NextSequence->GetPlayLength();
            if (NLen > 0.f)
                DamselProxy->NextTime =
                    FMath::Fmod(DamselProxy->NextTime + DeltaTime, NLen);
        }

        DamselProxy->BlendWeight = FMath::FInterpConstantTo(
            DamselProxy->BlendWeight, 1.f, DeltaTime, BlendInterpSpeed);

        if (DamselProxy->BlendWeight >= 0.999f)
        {
            DamselProxy->ActiveSequence = DamselProxy->NextSequence;
            DamselProxy->ActiveTime     = DamselProxy->NextTime;
            DamselProxy->NextSequence   = nullptr;
            DamselProxy->BlendWeight    = 0.f;
            DamselProxy->bInTransition  = false;
        }
        return;
    }

    // ── Advance active idle ───────────────────────────────────────────────────
    if (!DamselProxy->ActiveSequence) { return; }

    const float Len = DamselProxy->ActiveSequence->GetPlayLength();
    if (Len <= 0.f) { return; }

    const float PrevTime    = DamselProxy->ActiveTime;
    DamselProxy->ActiveTime = FMath::Fmod(PrevTime + DeltaTime, Len);

    // Loop detected → notify brain → blend to next idle.
    if (DamselProxy->ActiveTime < PrevTime)
    {
        if (Brain) { Brain->NotifyIdleLooped(); }
        BeginBlendToNextIdle();
    }
}

// =============================================================================
// BeginBlendToNextIdle
// =============================================================================
void UNPCDamselAnimInstance::BeginBlendToNextIdle()
{
    if (!DamselProxy || !Brain) { return; }

    int32 NextIdx = 0;
    switch (Brain->CurrentIdleState)
    {
    case EYoungWomanIdleState::Idle_A: NextIdx = 0; break;
    case EYoungWomanIdleState::Idle_B: NextIdx = 1; break;
    case EYoungWomanIdleState::Idle_C: NextIdx = 2; break;
    }

    UAnimSequence* NextSeq = GetIdleForIndex(NextIdx);
    if (!NextSeq || NextSeq == DamselProxy->ActiveSequence) { return; }

    DamselProxy->CurrentIdleIndex = NextIdx;
    DamselProxy->NextSequence     = NextSeq;
    DamselProxy->NextTime         = 0.f;
    DamselProxy->BlendWeight      = 0.f;
    DamselProxy->bInTransition    = true;
}
