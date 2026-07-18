#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "Character/NPC/YoungWoman/YoungWomanAnimInstance.h"
#include "Character/NPC/YoungWoman/YoungWomanFaceAnim.h"
#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"
#include "Character/NPC/YoungWoman/YoungWomanFaceAnimActorComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogYWGaze);

const FVector AYoungWoman::DefaultLookingPlaceRelativeLocation = FVector::ZeroVector;

// =============================================================================
// Constructor
// =============================================================================
AYoungWoman::AYoungWoman()
{
    PrimaryActorTick.bCanEverTick = false;

    bUseControllerRotationYaw   = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll  = false;

    if (UCharacterMovementComponent* MC = GetCharacterMovement())
    {
        MC->bOrientRotationToMovement = true;
        MC->RotationRate              = FRotator(0.f, RotationRate, 0.f);
        MC->MaxWalkSpeed              = WalkSpeed;
    }

    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetAnimInstanceClass(UYoungWomanAnimInstance::StaticClass());

    LookingPlace = CreateDefaultSubobject<USceneComponent>(TEXT("LookingPlace"));
    LookingPlace->SetupAttachment(GetRootComponent());
    LookingPlace->SetRelativeLocation(DefaultLookingPlaceRelativeLocation);

    FaceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FaceMesh"));
    FaceMesh->SetupAttachment(GetMesh());
    FaceMesh->SetAnimInstanceClass(UYoungWomanFaceAnim::StaticClass());
    FaceMesh->VisibilityBasedAnimTickOption =
        EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

    ClothesMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ClothesMesh"));
    ClothesMesh->SetupAttachment(GetMesh());

    CreateDefaultSubobject<UYoungWomanActorComponent>(TEXT("YoungWomanBrain"));
    CreateDefaultSubobject<UYoungWomanFaceAnimActorComponent>(TEXT("YoungWomanFaceAnimBrain"));
}

// =============================================================================
// BeginPlay
// =============================================================================
void AYoungWoman::BeginPlay()
{
    Super::BeginPlay();

    if (FaceMesh && FaceMesh->LeaderPoseComponent.IsValid())
    {
        FaceMesh->SetLeaderPoseComponent(nullptr);
    }

    if (UCharacterMovementComponent* MC = GetCharacterMovement())
    {
        MC->bOrientRotationToMovement = true;
        MC->RotationRate              = FRotator(0.f, RotationRate, 0.f);
        MC->MaxWalkSpeed              = WalkSpeed;
    }

    if (GetMesh() && AnimDataAsset)
    {
        HeadBoneIndex = GetMesh()->GetBoneIndex(AnimDataAsset->HeadBoneName);
    }
}

// =============================================================================
// Head look-at
// =============================================================================
void AYoungWoman::MoveLookingPlaceTo(const FVector& SarahHeadBoneWorldLocation)
{
    if (!LookingPlace) { return; }
    const float EyeZOffset = AnimDataAsset ? AnimDataAsset->LookAtEyeHeightOffset : 10.f;
    LookingPlace->SetWorldLocation(SarahHeadBoneWorldLocation + FVector(0.f, 0.f, EyeZOffset));
}

void AYoungWoman::StartLookAt()
{
    HeadLookAtAlpha = 1.f;
}

void AYoungWoman::StopLookAt()
{
    HeadLookAtAlpha = 0.f;
    if (LookingPlace)
    {
        LookingPlace->SetRelativeLocation(DefaultLookingPlaceRelativeLocation);
    }
}

// =============================================================================
// Bone transform accessors
// =============================================================================
FTransform AYoungWoman::GetHeadBoneWorldTransform() const
{
    if (HeadBoneIndex == INDEX_NONE || !GetMesh()) { return FTransform::Identity; }
    return GetMesh()->GetBoneTransform(HeadBoneIndex);
}
