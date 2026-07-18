#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "AnimNode_RigLogic.h"
#include "AnimNode_ControlRig.h"
#include "YoungWomanFaceAnim.generated.h"

class AYoungWoman;
class UYoungWomanFaceAnimActorComponent;
class UAnimSequence;
class UControlRig;

// ============================================================================
// FAnimNode_PosePassthrough
//
// Minimal helper node that forwards a pre-computed FPoseContext into the
// animation-node chain.  Injects pre-built poses into FAnimNode_ControlRig
// and FAnimNode_RigLogic without Blueprint-graph wiring.
// ============================================================================
struct FAnimNode_PosePassthrough : public FAnimNode_Base
{
    const FPoseContext* SourcePose = nullptr;

    virtual void Initialize_AnyThread(const FAnimationInitializeContext&) override {}
    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext&)  override {}
    virtual void Update_AnyThread(const FAnimationUpdateContext&)          override {}

    virtual void Evaluate_AnyThread(FPoseContext& Output) override
    {
        if (SourcePose)
        {
            Output.Pose.CopyBonesFrom(SourcePose->Pose);
            Output.Curve            = SourcePose->Curve;
            Output.CustomAttributes = SourcePose->CustomAttributes;
        }
        else
        {
            Output.ResetToRefPose();
        }
    }
};

// ============================================================================
// FYoungWomanFaceProxy — worker-thread proxy
//
// Full ABP_Face_PostProcess graph in C++:
//
//  CopyPoseFromMesh ─────────────────── body bone transforms
//  EvaluateFaceAnimPose ──────────────── greeting + blink CURVES
//                              Merge ──► InputPose
//                                            │
//         ┌─ HeadControlSwitch > 0.5         │
//         │  AND EnableHeadMovementIK        │
//         ▼                                 │
//    ControlRig ──────────── HeadMovedPose  │
//    CR_MetaHuman_HeadMovement_IK_Proc       │
//                                            │
//         ┌─ EnableFacialAnimation           │
//         ▼                                 │
//      RigLogic ─────────────────────────► Output
// ============================================================================
struct FYoungWomanFaceProxy : public FAnimInstanceProxy
{
public:
    FYoungWomanFaceProxy() = default;
    explicit FYoungWomanFaceProxy(UAnimInstance* InInstance)
        : FAnimInstanceProxy(InInstance) {}

    // ── Post-process pipeline nodes ───────────────────────────────────────────
    FAnimNode_CopyPoseFromMesh CopyPoseNode;
    FAnimNode_ControlRig       ControlRigNode;
    FAnimNode_RigLogic         RigLogicNode;
    FAnimNode_PosePassthrough  ControlRigInputPassthrough;
    FAnimNode_PosePassthrough  RigLogicInputPassthrough;

    // ── Variables synced from game thread every frame ─────────────────────────
    bool   bEnableFacialAnimation = true;
    int32  RigLogicLODThreshold   = -1;
    bool   bEnableHeadMovementIK  = false;
    float  HeadControlSwitchValue = 0.f;

    // ── Greeting sequence crossfade ───────────────────────────────────────────
    UAnimSequence* ActiveSequence = nullptr;
    float          ActiveTime     = 0.f;
    UAnimSequence* NextSequence   = nullptr;
    float          NextTime       = 0.f;
    float          BlendWeight    = 0.f;
    bool           bInTransition  = false;

    // ── Blink ─────────────────────────────────────────────────────────────────
    float BlinkAlpha             = 0.f;
    FName EyeBlinkLeftCurveName  = TEXT("CTRL_expressions_eyeBlinkL");
    FName EyeBlinkRightCurveName = TEXT("CTRL_expressions_eyeBlinkR");

    // ── FAnimInstanceProxy overrides ──────────────────────────────────────────
    virtual void Initialize(UAnimInstance* InAnimInstance) override;
    virtual void CacheBones()                              override;
    virtual void Update(float DeltaSeconds)                override;
    virtual bool Evaluate(FPoseContext& Output)            override;

private:
    void EvaluateFaceAnimPose(FPoseContext& OutPose);
    void ApplyBlinkCurves(FPoseContext& InOutPose);
};

// ============================================================================
// UYoungWomanFaceAnim — game-thread anim instance for AYoungWoman::FaceMesh
//
// Replaces ABP_Face_PostProcess entirely.
// LeaderPoseComponent must be null (cleared in AYoungWoman::BeginPlay).
// ============================================================================
UCLASS()
class GAME_API UYoungWomanFaceAnim : public UAnimInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face|RigLogic",
        meta = (DisplayName = "Enable Facial Animation"))
    bool bEnableFacialAnimation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face|RigLogic")
    int32 RigLogicLODThreshold = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face|HeadIK")
    bool bEnableHeadMovementIK = false;

    virtual void NativeInitializeAnimation()            override;
    virtual void NativeUpdateAnimation(float DeltaTime) override;

    /** Called by UYoungWomanFaceAnimActorComponent to start a greeting anim. */
    void PlayGreetingFromAC(int32 Index, UAnimSequence* Seq);

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy()                             override;
    virtual void                DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
    FYoungWomanFaceProxy* FaceProxy = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UYoungWomanFaceAnimActorComponent> FaceAC;

    UPROPERTY(Transient)
    TObjectPtr<AYoungWoman> CachedYW;

    // ── Greeting ──────────────────────────────────────────────────────────────
    float BlendInterpSpeed = 4.f;
    bool  bGreetingActive  = false;
    bool  bInHoldDelay     = false;
    float HoldTimer        = 0.f;

    static constexpr float kHoldDuration = 1.0f;

    void TickTransition(float DeltaTime);
    void FinishGreeting();

    // ── Blink ─────────────────────────────────────────────────────────────────
    bool  bEyesClosed          = false;
    float TargetBlinkAlpha     = 0.f;
    float CurrentBlinkAlpha    = 0.f;
    float BlinkAlphaBlendSpeed = 10.f;
    float BlinkClosedMin       = 0.10f;
    float BlinkClosedMax       = 0.25f;
    float BlinkOpenMin         = 2.0f;
    float BlinkOpenMax         = 4.0f;

    FTimerHandle BlinkTimerHandle;

    void StartBlinkLoop();
    void ScheduleBlinkPhase(float MinTime, float MaxTime);
    void OnBlinkPhaseTimer();
};
