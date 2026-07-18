#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/NPCCharacter.h"
#include "Data/DataAssets/Animation/NPC/YoungWoman/YoungWomanAnimationDataAsset.h"
#include "Character/NPC/YoungWoman/YoungWomanDataAsset.h"
#include "YoungWoman.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogYWGaze, Warning, All);

UCLASS()
class GAME_API AYoungWoman : public ANPCCharacter
{
    GENERATED_BODY()

public:
    AYoungWoman();

    // ── Movement ──────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RunSpeed = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RotationRate = 720.f;

    // ── Data assets ───────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TObjectPtr<UYoungWomanAnimationDataAsset> AnimDataAsset = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
    TObjectPtr<UYoungWomanDataAsset> PersonalityDataAsset = nullptr;

    // ── Head look-at ──────────────────────────────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|LookAt")
    TObjectPtr<USceneComponent> LookingPlace;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|LookAt")
    float HeadLookAtAlpha = 0.f;

    void MoveLookingPlaceTo(const FVector& SarahHeadBoneWorldLocation);
    void StartLookAt();
    void StopLookAt();

    // ── Mesh accessors ────────────────────────────────────────────────────────

    FORCEINLINE USkeletalMeshComponent* GetBodyMesh()    const { return GetMesh(); }
    FORCEINLINE USkeletalMeshComponent* GetFaceMesh()    const { return FaceMesh; }
    FORCEINLINE USkeletalMeshComponent* GetClothesMesh() const { return ClothesMesh; }

    // ── Head bone (body mesh) ─────────────────────────────────────────────────

    FTransform GetHeadBoneWorldTransform() const;
    FORCEINLINE int32 GetHeadBoneIndex() const { return HeadBoneIndex; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Face",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> FaceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Clothes",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> ClothesMesh;

    int32 HeadBoneIndex = INDEX_NONE;

    static const FVector DefaultLookingPlaceRelativeLocation;
};
