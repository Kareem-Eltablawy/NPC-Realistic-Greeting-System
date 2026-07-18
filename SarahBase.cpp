#include "Character/Sarah/SarahBase.h"
#include "Character/Sarah/Interaction/SarahInteraction.h"
#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"

static const FName SARAH_HEAD_BONE = TEXT("head");

// =============================================================================
// Constructor
// =============================================================================
ASarahBase::ASarahBase()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionSphere =
        CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionSphere->SetGenerateOverlapEvents(true);

    InteractionSphere->OnComponentBeginOverlap.AddDynamic(
        this, &ASarahBase::OnInteractionSphereBeginOverlap);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(
        this, &ASarahBase::OnInteractionSphereEndOverlap);
}

// =============================================================================
// BeginPlay
// =============================================================================
void ASarahBase::BeginPlay()
{
    Super::BeginPlay();

    if (DataAsset)
    {
        CachedHitOverlapRadius = DataAsset->HitOverlapRadius;

        if (DataAsset->InteractionRadius > 0.f)
        {
            InteractionSphere->SetSphereRadius(DataAsset->InteractionRadius);
        }
    }

    InteractionComp = FindComponentByClass<USarahInteraction>();
}

// =============================================================================
// SetupPlayerInputComponent
// =============================================================================
void ASarahBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// =============================================================================
// GetSensoryOrigin
// =============================================================================
FVector ASarahBase::GetSensoryOrigin() const
{
    const float ZOffset = DataAsset ? DataAsset->SensoryOriginZOffset : 70.f;
    return GetActorLocation() + FVector(0.f, 0.f, ZOffset);
}

// =============================================================================
// Overlap — NPC entered sphere
// =============================================================================
void ASarahBase::OnInteractionSphereBeginOverlap(UPrimitiveComponent*,
    AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    AYoungWoman* YW = Cast<AYoungWoman>(OtherActor);
    if (!YW) { return; }
    if (InteractionState == ESarahInteractionState::NearNPC) { return; }

    CurrentYoungWoman = YW;
    InteractionState  = ESarahInteractionState::NearNPC;

    if (InteractionComp) { InteractionComp->OnSarahEnteredNPCRange(YW); }
}

// =============================================================================
// Overlap — NPC left sphere
// =============================================================================
void ASarahBase::OnInteractionSphereEndOverlap(UPrimitiveComponent*,
    AActor* OtherActor, UPrimitiveComponent*, int32)
{
    if (!CurrentYoungWoman.IsValid()) { return; }
    if (CurrentYoungWoman.Get() != OtherActor) { return; }

    if (UYoungWomanActorComponent* Brain =
        CurrentYoungWoman->FindComponentByClass<UYoungWomanActorComponent>())
    {
        Brain->OnSarahLeft();
    }

    CurrentYoungWoman = nullptr;
    InteractionState  = ESarahInteractionState::Idle;

    if (InteractionComp) { InteractionComp->OnSarahLeftNPCRange(); }
}

// =============================================================================
// TryInteract  (G key)
// =============================================================================
void ASarahBase::TryInteract()
{
    if (InteractionState != ESarahInteractionState::NearNPC) { return; }
    if (!CurrentYoungWoman.IsValid()) { return; }
    if (InteractionComp) { InteractionComp->PlaySarahGreeting(); }
}

// =============================================================================
// OnSarahGreetingEnded  (called by USarahInteraction)
// =============================================================================
void ASarahBase::OnSarahGreetingEnded()
{
    if (InteractionState != ESarahInteractionState::NearNPC) { return; }

    AYoungWoman* YW = CurrentYoungWoman.Get();
    if (!YW) { return; }

    if (UYoungWomanActorComponent* Brain =
        YW->FindComponentByClass<UYoungWomanActorComponent>())
    {
        USkeletalMeshComponent* SarahBodyMesh = GetMesh();
        if (SarahBodyMesh)
        {
            Brain->OnSarahGreeted(SarahBodyMesh->GetBoneLocation(SARAH_HEAD_BONE));
        }
    }
}
