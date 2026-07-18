#include "Character/NPC/YoungWoman/YoungWomanAnimInstance.h"
#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimNodeBase.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"

// =============================================================================
// File-scope bone-axis helper
// =============================================================================
static FORCEINLINE FVector GetBoneAxisVector(const FQuat& Rot, EYoungWomanBoneAxis Axis)
{
    switch (Axis)
    {
    case EYoungWomanBoneAxis::X:     return  Rot.GetAxisX();
    case EYoungWomanBoneAxis::X_Neg: return -Rot.GetAxisX();
    case EYoungWomanBoneAxis::Y:     return  Rot.GetAxisY();
    case EYoungWomanBoneAxis::Y_Neg: return -Rot.GetAxisY();
    case EYoungWomanBoneAxis::Z:     return  Rot.GetAxisZ();
    case EYoungWomanBoneAxis::Z_Neg: return -Rot.GetAxisZ();
    default:                         return  Rot.GetAxisX();
    }
}

// =============================================================================
// UYoungWomanAnimInstance
// =============================================================================
UYoungWomanAnimInstance::UYoungWomanAnimInstance()
{}

void UYoungWomanAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    YoungWoman = Cast<AYoungWoman>(TryGetPawnOwner());
    MoveComp   = YoungWoman ? YoungWoman->GetCharacterMovement()                              : nullptr;
    Brain      = YoungWoman ? YoungWoman->FindComponentByClass<UYoungWomanActorComponent>() : nullptr;
    MyProxy    = &GetProxyOnGameThread<FYoungWomanAnimProxy>();

    CurrentLookAtAlpha = 0.f;
}

// =============================================================================
// NativeUpdateAnimation
// =============================================================================
void UYoungWomanAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);
    if (!YoungWoman || !MoveComp || !MyProxy) { return; }

    // ── Locomotion ────────────────────────────────────────────────────────────
    Speed     = YoungWoman->GetVelocity().Size2D();
    bIsMoving = Speed > 10.f;

    // ── Smooth the head LookAt alpha ──────────────────────────────────────────
    const float RawTargetAlpha  = YoungWoman->HeadLookAtAlpha;
    const float AlphaBlendSpeed = YoungWoman->AnimDataAsset
        ? FMath::Max(YoungWoman->AnimDataAsset->LookAtAlphaBlendSpeed, 0.1f)
        : 3.f;

    CurrentLookAtAlpha = (DeltaTime > 0.f)
        ? FMath::FInterpConstantTo(CurrentLookAtAlpha, RawTargetAlpha, DeltaTime, AlphaBlendSpeed)
        : RawTargetAlpha;

    MyProxy->LookAtAlpha = CurrentLookAtAlpha;

    // ── Push LookAt target to proxy ───────────────────────────────────────────
    if (RawTargetAlpha > KINDA_SMALL_NUMBER)
    {
        if (USceneComponent* LP = YoungWoman->LookingPlace)
        {
            MyProxy->LookAtTargetWorld = LP->GetComponentLocation();
        }
    }

    if (MyProxy->CachedHeadBoneIndex == INDEX_NONE)
    {
        MyProxy->CachedHeadBoneIndex = YoungWoman->GetHeadBoneIndex();
    }

    if (YoungWoman->AnimDataAsset)
    {
        MyProxy->CachedLookAtAxis = YoungWoman->AnimDataAsset->LookAtAxis;
        MyProxy->LookAtTrackingSpeedRadPerSec =
            FMath::DegreesToRadians(
                FMath::Max(YoungWoman->AnimDataAsset->HeadLookAtTrackingSpeed, 10.f));
    }

    MyProxy->DeltaTimeForLookAt = DeltaTime;

    // Reset smoothed direction when fully idle so the next LookAt starts clean.
    if (CurrentLookAtAlpha <= KINDA_SMALL_NUMBER && RawTargetAlpha <= KINDA_SMALL_NUMBER)
    {
        MyProxy->SmoothedLookDirCS = FVector::ZeroVector;
    }

    // ── Suppress body post-process ABP's hardcoded head movement ─────────────
    if (bSuppressSharedPostProcessLookAt)
    {
        SuppressSharedPostProcessHeadMovementControl();
    }
}

// =============================================================================
// SuppressSharedPostProcessHeadMovementControl
// =============================================================================
void UYoungWomanAnimInstance::SuppressSharedPostProcessHeadMovementControl()
{
    USkeletalMeshComponent* BodyMesh = YoungWoman ? YoungWoman->GetMesh() : nullptr;
    if (!BodyMesh) { return; }

    UAnimInstance* PostProcessInst = BodyMesh->GetPostProcessInstance();
    if (!PostProcessInst) { return; }

    if (CachedPostProcessInstanceForProp.Get() != PostProcessInst)
    {
        CachedPostProcessInstanceForProp = PostProcessInst;
        CachedHeadMovementControlProp    =
            PostProcessInst->GetClass()->FindPropertyByName(
                TEXT("Head Movement Control"));
    }

    if (FDoubleProperty* DoubleProp =
        CastField<FDoubleProperty>(CachedHeadMovementControlProp))
    {
        DoubleProp->SetPropertyValue_InContainer(PostProcessInst, 0.0);
    }
    else if (FFloatProperty* FloatProp =
        CastField<FFloatProperty>(CachedHeadMovementControlProp))
    {
        FloatProp->SetPropertyValue_InContainer(PostProcessInst, 0.f);
    }
}

// =============================================================================
// NativePostEvaluateAnimation
// =============================================================================
void UYoungWomanAnimInstance::NativePostEvaluateAnimation()
{
    Super::NativePostEvaluateAnimation();
}

// =============================================================================
// Proxy factory
// =============================================================================
FAnimInstanceProxy* UYoungWomanAnimInstance::CreateAnimInstanceProxy()
{
    MyProxy = new FYoungWomanAnimProxy(this);
    return MyProxy;
}

void UYoungWomanAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
    MyProxy = nullptr;
    delete InProxy;
}

// =============================================================================
// FYoungWomanAnimProxy::Evaluate
// =============================================================================
bool FYoungWomanAnimProxy::Evaluate(FPoseContext& Output)
{
    const bool bEvaluated = FAnimInstanceProxy::Evaluate(Output);

    if (LookAtAlpha > KINDA_SMALL_NUMBER && CachedHeadBoneIndex != INDEX_NONE)
    {
        ApplyLookAt(Output);
    }

    return bEvaluated;
}

// =============================================================================
// FYoungWomanAnimProxy::ApplyLookAt
// =============================================================================
void FYoungWomanAnimProxy::ApplyLookAt(FPoseContext& Output)
{
    const FBoneContainer& BC = Output.Pose.GetBoneContainer();
    const FCompactPoseBoneIndex CompactHeadIdx =
        BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(CachedHeadBoneIndex));

    if (!CompactHeadIdx.IsValid()) { return; }

    FCSPose<FCompactPose> CSPose;
    CSPose.InitPose(Output.Pose);

    const FTransform  HeadCS      = CSPose.GetComponentSpaceTransform(CompactHeadIdx);
    const FTransform& CompToWorld = GetComponentTransform();

    const FVector TargetCSv        = CompToWorld.InverseTransformPosition(LookAtTargetWorld);
    FVector       DesiredLookDirCS = (TargetCSv - HeadCS.GetLocation()).GetSafeNormal();

    if (DesiredLookDirCS.IsNearlyZero()) { return; }

    const FVector CurrentLookCS = GetBoneAxisVector(HeadCS.GetRotation(), CachedLookAtAxis);

    if (SmoothedLookDirCS.IsZero()) { SmoothedLookDirCS = CurrentLookCS; }

    // Clamp to 60 degrees from rest direction.
    constexpr float ClampRad = PI / 3.f;

    const float AngleToDesired = FMath::Acos(
        FMath::Clamp(FVector::DotProduct(CurrentLookCS, DesiredLookDirCS), -1.f, 1.f));

    if (AngleToDesired > ClampRad)
    {
        const FVector ClampAxis =
            FVector::CrossProduct(CurrentLookCS, DesiredLookDirCS).GetSafeNormal();
        if (ClampAxis.IsNearlyZero()) { return; }
        DesiredLookDirCS =
            FQuat(ClampAxis, ClampRad).RotateVector(CurrentLookCS).GetSafeNormal();
    }

    // Angular-speed slerp.
    if (DeltaTimeForLookAt > 0.f)
    {
        const float AngleRemaining = FMath::Acos(
            FMath::Clamp(
                FVector::DotProduct(SmoothedLookDirCS, DesiredLookDirCS), -1.f, 1.f));

        if (AngleRemaining > KINDA_SMALL_NUMBER)
        {
            const float MaxStep = LookAtTrackingSpeedRadPerSec * DeltaTimeForLookAt;
            if (AngleRemaining > MaxStep)
            {
                const FVector InterpAxis =
                    FVector::CrossProduct(SmoothedLookDirCS, DesiredLookDirCS).GetSafeNormal();
                if (!InterpAxis.IsNearlyZero())
                {
                    SmoothedLookDirCS =
                        FQuat(InterpAxis, MaxStep).RotateVector(SmoothedLookDirCS).GetSafeNormal();
                }
            }
            else
            {
                SmoothedLookDirCS = DesiredLookDirCS;
            }
        }
    }
    else
    {
        SmoothedLookDirCS = DesiredLookDirCS;
    }

    // Build and blend the final head rotation.
    const FQuat DeltaRot    = FQuat::FindBetweenNormals(CurrentLookCS, SmoothedLookDirCS);
    FQuat TargetCSQuat      = (DeltaRot * HeadCS.GetRotation()).GetNormalized();
    FQuat BlendedCSQuat     = FQuat::Slerp(HeadCS.GetRotation(), TargetCSQuat, LookAtAlpha);
    BlendedCSQuat.Normalize();

    FTransform NewHeadCS = HeadCS;
    NewHeadCS.SetRotation(BlendedCSQuat);

    const FCompactPoseBoneIndex ParentIdx = BC.GetParentBoneIndex(CompactHeadIdx);
    if (ParentIdx.IsValid())
    {
        const FTransform ParentCS = CSPose.GetComponentSpaceTransform(ParentIdx);
        Output.Pose[CompactHeadIdx] = NewHeadCS.GetRelativeTransform(ParentCS);
    }
    else
    {
        Output.Pose[CompactHeadIdx] = NewHeadCS;
    }
}
