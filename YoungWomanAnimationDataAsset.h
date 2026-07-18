#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"
#include "YoungWomanAnimationDataAsset.generated.h"

UENUM(BlueprintType)
enum class EYoungWomanHospitality : uint8
{
    Friendly        UMETA(DisplayName = "Friendly (0.5-1 s)"),
    Slower          UMETA(DisplayName = "Slower (1-2 s)"),
    LessFriendly    UMETA(DisplayName = "Less Friendly (2-3 s)"),
};

UENUM(BlueprintType)
enum class EYoungWomanBoneAxis : uint8
{
    X       UMETA(DisplayName = "X"),
    X_Neg   UMETA(DisplayName = "-X"),
    Y       UMETA(DisplayName = "Y (MetaHuman face-forward)"),
    Y_Neg   UMETA(DisplayName = "-Y"),
    Z       UMETA(DisplayName = "Z"),
    Z_Neg   UMETA(DisplayName = "-Z"),
};

FORCEINLINE static FVector GetYoungWomanBoneAxisVector(const FQuat& Rot, EYoungWomanBoneAxis Axis)
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

USTRUCT(BlueprintType)
struct FYoungWomanGreeting
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Greeting")
    TSoftObjectPtr<UAnimSequence> Animation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Greeting")
    TSoftObjectPtr<USoundBase> Audio;
};

UENUM(BlueprintType)
enum class EYoungWomanGreetingSpeed : uint8
{
    Instant  UMETA(DisplayName = "Instant"),
    Fast     UMETA(DisplayName = "Fast"),
    Slow     UMETA(DisplayName = "Slow"),
    Slower   UMETA(DisplayName = "Slower"),
};

UENUM(BlueprintType)
enum class EYoungWomanGreetingCaring : uint8
{
    Caring    UMETA(DisplayName = "Caring"),
    NotCaring UMETA(DisplayName = "Not Caring"),
};

UCLASS(BlueprintType)
class GAME_API UYoungWomanAnimationDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // ── Greetings ─────────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Greetings")
    TArray<FYoungWomanGreeting> Greetings;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Greetings",
        meta = (ClampMin = "0.5", ClampMax = "30.0"))
    float FaceBlendInterpSpeed = 4.0f;

    // ── Interaction ───────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction")
    EYoungWomanHospitality Hospitality = EYoungWomanHospitality::Friendly;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction|LookAt")
    FVector PlaceOfViewOffset = FVector(0.f, 0.f, 170.f);

    // ── StartGreeting ─────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StartGreeting")
    TArray<TSoftObjectPtr<UAnimSequence>> StartGreetingAnimations;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StartGreeting")
    TArray<TSoftObjectPtr<USoundBase>> StartGreetingAudios;

    // ── Body mesh — head bone ─────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction|LookAt")
    FName HeadBoneName = TEXT("head");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction|LookAt")
    EYoungWomanBoneAxis LookAtAxis = EYoungWomanBoneAxis::Y;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction|LookAt",
        meta = (ClampMin = "-50.0", ClampMax = "50.0"))
    float LookAtEyeHeightOffset = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction|LookAt",
        meta = (ClampMin = "10.0", ClampMax = "720.0"))
    float HeadLookAtTrackingSpeed = 360.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Interaction|LookAt",
        meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float LookAtAlphaBlendSpeed = 3.0f;

    // ── Blinking ──────────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink",
        meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float BlinkClosedDurationMin = 0.10f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink",
        meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float BlinkClosedDurationMax = 0.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink",
        meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float BlinkOpenDurationMin = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink",
        meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float BlinkOpenDurationMax = 4.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink",
        meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float BlinkAlphaBlendSpeed = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink")
    FName EyeBlinkLeftCurveName = TEXT("CTRL_expressions_eyeBlinkL");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YoungWoman|Face|Blink")
    FName EyeBlinkRightCurveName = TEXT("CTRL_expressions_eyeBlinkR");

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("YoungWomanAnimationDataAsset", GetFName());
    }
};
