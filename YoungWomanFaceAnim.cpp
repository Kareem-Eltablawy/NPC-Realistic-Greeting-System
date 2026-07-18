#include "Character/NPC/YoungWoman/YoungWomanFaceAnim.h"
#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "Character/NPC/YoungWoman/YoungWomanFaceAnimActorComponent.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "ControlRig.h"
#include "AlphaBlend.h"
#include "UObject/UnrealType.h"

// =============================================================================
// FYoungWomanFaceProxy :: Initialize
// =============================================================================
void FYoungWomanFaceProxy::Initialize(UAnimInstance* InAnimInstance)
{
    FAnimInstanceProxy::Initialize(InAnimInstance);

    CopyPoseNode.bUseAttachedParent = true;
    CopyPoseNode.bCopyCurves        = true;
    CopyPoseNode.PreUpdate(InAnimInstance);

    FAnimationInitializeContext Context(this);
    CopyPoseNode.Initialize_AnyThread(Context);
    ControlRigNode.Initialize_AnyThread(Context);
    RigLogicNode.Initialize_AnyThread(Context);
}

// =============================================================================
// FYoungWomanFaceProxy :: CacheBones
// =============================================================================
void FYoungWomanFaceProxy::CacheBones()
{
    FAnimInstanceProxy::CacheBones();

    FAnimationCacheBonesContext Context(this);
    CopyPoseNode.CacheBones_AnyThread(Context);
    ControlRigNode.CacheBones_AnyThread(Context);
    RigLogicNode.CacheBones_AnyThread(Context);
}

// =============================================================================
// FYoungWomanFaceProxy :: Update
// =============================================================================
void FYoungWomanFaceProxy::Update(float DeltaSeconds)
{
    FAnimInstanceProxy::Update(DeltaSeconds);

    FAnimationUpdateContext Context(this, DeltaSeconds);

    const bool bRunHeadIK = bEnableHeadMovementIK && (HeadControlSwitchValue > 0.5f);

    CopyPoseNode.Update_AnyThread(Context);
    if (bRunHeadIK)             { ControlRigNode.Update_AnyThread(Context); }
    if (bEnableFacialAnimation) { RigLogicNode.Update_AnyThread(Context); }
}

// =============================================================================
// FYoungWomanFaceProxy :: Evaluate
// =============================================================================
bool FYoungWomanFaceProxy::Evaluate(FPoseContext& Output)
{
    Output.ResetToRefPose();

    // Stage 1 — CopyPoseFromMesh (body bone transforms)
    FPoseContext BodyCtx(this);
    CopyPoseNode.Evaluate_AnyThread(BodyCtx);

    // Stage 2 — Face animation pose (greeting crossfade + blink curves)
    FPoseContext FaceAnimCtx(this);
    FaceAnimCtx.ResetToRefPose();
    EvaluateFaceAnimPose(FaceAnimCtx);

    // Stage 3 — Merge: body bones + face curves → InputPose
    FPoseContext InputPoseCtx(this);
    InputPoseCtx.ResetToRefPose();
    InputPoseCtx.Pose.CopyBonesFrom(BodyCtx.Pose);
    InputPoseCtx.Curve            = FaceAnimCtx.Curve;
    InputPoseCtx.CustomAttributes = FaceAnimCtx.CustomAttributes;

    // Stage 4 — ControlRig head movement IK (conditional)
    const bool bRunHeadIK = bEnableHeadMovementIK && (HeadControlSwitchValue > 0.5f);

    FPoseContext HeadMovedCtx(this);
    if (bRunHeadIK)
    {
        ControlRigInputPassthrough.SourcePose = &InputPoseCtx;
        ControlRigNode.Evaluate_AnyThread(HeadMovedCtx);
    }
    else
    {
        HeadMovedCtx.Pose.CopyBonesFrom(InputPoseCtx.Pose);
        HeadMovedCtx.Curve            = InputPoseCtx.Curve;
        HeadMovedCtx.CustomAttributes = InputPoseCtx.CustomAttributes;
    }

    // Stage 5 — RigLogic MetaHuman face deformation (conditional)
    if (bEnableFacialAnimation)
    {
        RigLogicInputPassthrough.SourcePose = &HeadMovedCtx;
        RigLogicNode.Evaluate_AnyThread(Output);
    }
    else
    {
        Output.Pose.CopyBonesFrom(HeadMovedCtx.Pose);
        Output.Curve            = HeadMovedCtx.Curve;
        Output.CustomAttributes = HeadMovedCtx.CustomAttributes;
    }

    return true;
}

// =============================================================================
// FYoungWomanFaceProxy :: EvaluateFaceAnimPose
// =============================================================================
void FYoungWomanFaceProxy::EvaluateFaceAnimPose(FPoseContext& OutPose)
{
    if (ActiveSequence)
    {
        const FBoneContainer& BC = OutPose.Pose.GetBoneContainer();

        FCompactPose AP; FBlendedCurve AC; UE::Anim::FStackAttributeContainer AA;
        AP.SetBoneContainer(&BC); AC.InitFrom(BC);
        FAnimationPoseData AD(AP, AC, AA);
        ActiveSequence->GetAnimationPose(AD,
            FAnimExtractContext((double)ActiveTime, false));

        if (!NextSequence || BlendWeight <= KINDA_SMALL_NUMBER)
        {
            OutPose.Pose  = AP;
            OutPose.Curve = AC;
        }
        else
        {
            FCompactPose NP; FBlendedCurve NC; UE::Anim::FStackAttributeContainer NA;
            NP.SetBoneContainer(&BC); NC.InitFrom(BC);
            FAnimationPoseData ND(NP, NC, NA);
            NextSequence->GetAnimationPose(ND,
                FAnimExtractContext((double)NextTime, false));

            UE::Anim::FStackAttributeContainer OA;
            FAnimationPoseData OD(OutPose.Pose, OutPose.Curve, OA);
            FAnimationRuntime::BlendTwoPosesTogether(AD, ND, 1.0f - BlendWeight, OD);
        }
    }

    ApplyBlinkCurves(OutPose);
}

// =============================================================================
// FYoungWomanFaceProxy :: ApplyBlinkCurves
// =============================================================================
void FYoungWomanFaceProxy::ApplyBlinkCurves(FPoseContext& InOutPose)
{
    if (BlinkAlpha <= KINDA_SMALL_NUMBER) { return; }

    auto Inject = [&](const FName& Name)
    {
        if (!Name.IsNone()) { InOutPose.Curve.Set(Name, BlinkAlpha); }
    };
    Inject(EyeBlinkLeftCurveName);
    Inject(EyeBlinkRightCurveName);
}

// =============================================================================
// Proxy lifetime
// =============================================================================
FAnimInstanceProxy* UYoungWomanFaceAnim::CreateAnimInstanceProxy()
{
    FaceProxy = new FYoungWomanFaceProxy(this);
    return FaceProxy;
}

void UYoungWomanFaceAnim::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
    FaceProxy = nullptr;
    delete InProxy;
}

// =============================================================================
// NativeInitializeAnimation
// =============================================================================
void UYoungWomanFaceAnim::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    bGreetingActive   = false;
    bInHoldDelay      = false;
    HoldTimer         = 0.f;
    FaceAC            = nullptr;
    CachedYW          = nullptr;
    bEyesClosed       = false;
    TargetBlinkAlpha  = 0.f;
    CurrentBlinkAlpha = 0.f;

    if (FaceProxy)
    {
        FaceProxy->ActiveSequence     = nullptr;
        FaceProxy->NextSequence       = nullptr;
        FaceProxy->ActiveTime         = 0.f;
        FaceProxy->NextTime           = 0.f;
        FaceProxy->BlendWeight        = 0.f;
        FaceProxy->bInTransition      = false;
        FaceProxy->BlinkAlpha         = 0.f;
        FaceProxy->bEnableFacialAnimation = bEnableFacialAnimation;
        FaceProxy->RigLogicLODThreshold   = RigLogicLODThreshold;
        FaceProxy->bEnableHeadMovementIK  = bEnableHeadMovementIK;
    }

    if (APawn* Pawn = TryGetPawnOwner())
        CachedYW = Cast<AYoungWoman>(Pawn);
    if (!IsValid(CachedYW))
        if (USkeletalMeshComponent* Comp = GetSkelMeshComponent())
            CachedYW = Cast<AYoungWoman>(Comp->GetOwner());

    if (!IsValid(CachedYW)) { StartBlinkLoop(); return; }

    FaceAC = CachedYW->FindComponentByClass<UYoungWomanFaceAnimActorComponent>();

    const UYoungWomanAnimationDataAsset* DA = CachedYW->AnimDataAsset;
    if (!DA) { StartBlinkLoop(); return; }

    BlendInterpSpeed     = DA->FaceBlendInterpSpeed;
    BlinkAlphaBlendSpeed = DA->BlinkAlphaBlendSpeed;
    BlinkClosedMin       = DA->BlinkClosedDurationMin;
    BlinkClosedMax       = DA->BlinkClosedDurationMax;
    BlinkOpenMin         = DA->BlinkOpenDurationMin;
    BlinkOpenMax         = DA->BlinkOpenDurationMax;

    if (FaceProxy)
    {
        FaceProxy->EyeBlinkLeftCurveName  = DA->EyeBlinkLeftCurveName;
        FaceProxy->EyeBlinkRightCurveName = DA->EyeBlinkRightCurveName;
    }

    if (USkeletalMeshComponent* FaceComp = GetSkelMeshComponent())
        if (FaceComp->LeaderPoseComponent.IsValid())
            FaceComp->SetLeaderPoseComponent(nullptr);

    StartBlinkLoop();
}

// =============================================================================
// NativeUpdateAnimation
// =============================================================================
void UYoungWomanFaceAnim::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);
    if (!FaceProxy) { return; }

    FaceProxy->CopyPoseNode.PreUpdate(this);
    FaceProxy->bEnableFacialAnimation = bEnableFacialAnimation;
    FaceProxy->RigLogicLODThreshold   = RigLogicLODThreshold;
    FaceProxy->bEnableHeadMovementIK  = bEnableHeadMovementIK;
    FaceProxy->HeadControlSwitchValue = GetCurveValue(TEXT("HeadControlSwitch"));

    if (DeltaTime > 0.f)
    {
        CurrentBlinkAlpha = FMath::FInterpConstantTo(
            CurrentBlinkAlpha, TargetBlinkAlpha, DeltaTime, BlinkAlphaBlendSpeed);
    }
    FaceProxy->BlinkAlpha = CurrentBlinkAlpha;

    if (FaceProxy->bInTransition) { TickTransition(DeltaTime); return; }

    if (bInHoldDelay)
    {
        HoldTimer += DeltaTime;
        if (HoldTimer >= kHoldDuration)
        {
            bInHoldDelay = false;
            HoldTimer    = 0.f;
            FinishGreeting();
        }
        return;
    }

    if (!FaceProxy->ActiveSequence) { return; }
    const float Len = FaceProxy->ActiveSequence->GetPlayLength();
    if (Len <= 0.f) { return; }

    FaceProxy->ActiveTime += DeltaTime;

    if (bGreetingActive && FaceProxy->ActiveTime >= Len)
    {
        FaceProxy->ActiveTime = Len - SMALL_NUMBER;
        bGreetingActive       = false;
        bInHoldDelay          = true;
        HoldTimer             = 0.f;
    }
}

// =============================================================================
// TickTransition
// =============================================================================
void UYoungWomanFaceAnim::TickTransition(float DeltaTime)
{
    if (!FaceProxy->NextSequence) { FaceProxy->bInTransition = false; return; }

    const float NLen = FaceProxy->NextSequence->GetPlayLength();
    if (NLen > 0.f)
        FaceProxy->NextTime =
            FMath::Min(FaceProxy->NextTime + DeltaTime, NLen - SMALL_NUMBER);

    FaceProxy->BlendWeight = FMath::FInterpConstantTo(
        FaceProxy->BlendWeight, 1.f, DeltaTime, BlendInterpSpeed);

    if (FaceProxy->BlendWeight >= 0.999f)
    {
        FaceProxy->ActiveSequence = FaceProxy->NextSequence;
        FaceProxy->ActiveTime     = FaceProxy->NextTime;
        FaceProxy->NextSequence   = nullptr;
        FaceProxy->BlendWeight    = 0.f;
        FaceProxy->bInTransition  = false;
    }
}

// =============================================================================
// FinishGreeting
// =============================================================================
void UYoungWomanFaceAnim::FinishGreeting()
{
    if (FaceProxy)
    {
        FaceProxy->ActiveSequence = nullptr;
        FaceProxy->ActiveTime     = 0.f;
    }
    if (FaceAC) { FaceAC->NotifyFaceAnimEnded(); }
}

// =============================================================================
// PlayGreetingFromAC
// =============================================================================
void UYoungWomanFaceAnim::PlayGreetingFromAC(int32 Index, UAnimSequence* Seq)
{
    if (!FaceProxy || !Seq) { return; }

    bInHoldDelay    = false;
    HoldTimer       = 0.f;
    bGreetingActive = true;

    if (!FaceProxy->ActiveSequence)
    {
        FaceProxy->ActiveSequence = Seq;
        FaceProxy->ActiveTime     = 0.f;
        FaceProxy->NextSequence   = nullptr;
        FaceProxy->BlendWeight    = 0.f;
        FaceProxy->bInTransition  = false;
    }
    else
    {
        FaceProxy->NextSequence  = Seq;
        FaceProxy->NextTime      = 0.f;
        FaceProxy->BlendWeight   = 0.f;
        FaceProxy->bInTransition = true;
    }
}

// =============================================================================
// Blink loop
// =============================================================================
void UYoungWomanFaceAnim::StartBlinkLoop()
{
    if (UWorld* W = GetWorld()) { W->GetTimerManager().ClearTimer(BlinkTimerHandle); }
    bEyesClosed      = false;
    TargetBlinkAlpha = 0.f;
    ScheduleBlinkPhase(BlinkOpenMin, BlinkOpenMax);
}

void UYoungWomanFaceAnim::ScheduleBlinkPhase(float MinTime, float MaxTime)
{
    UWorld* W = GetWorld();
    if (!W) { return; }
    const float Delay = FMath::FRandRange(MinTime, MaxTime);
    W->GetTimerManager().SetTimer(
        BlinkTimerHandle, this,
        &UYoungWomanFaceAnim::OnBlinkPhaseTimer, Delay, false);
}

void UYoungWomanFaceAnim::OnBlinkPhaseTimer()
{
    if (!bEyesClosed)
    {
        bEyesClosed      = true;
        TargetBlinkAlpha = 1.f;
        ScheduleBlinkPhase(BlinkClosedMin, BlinkClosedMax);
    }
    else
    {
        bEyesClosed      = false;
        TargetBlinkAlpha = 0.f;
        ScheduleBlinkPhase(BlinkOpenMin, BlinkOpenMax);
    }
}
