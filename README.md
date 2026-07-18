# YoungWoman NPC System — Unreal Engine 5 · Native C++

> **Portfolio extract from *Moon* — a narrative action RPG set in Crusades-era Beirut.**
> This module implements a full MetaHuman NPC pipeline in native C++ with zero Blueprint gameplay logic.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [File Reference](#file-reference)
- [Key Technical Decisions](#key-technical-decisions)
- [System Walkthrough](#system-walkthrough)
- [Data Asset Configuration](#data-asset-configuration)
- [Integration Guide](#integration-guide)
- [Engine & Dependencies](#engine--dependencies)

---

## Overview

The YoungWoman system is a self-contained NPC module that handles:

- **Three-mesh MetaHuman rendering** — body, face, and clothes as separate `USkeletalMeshComponent` instances driven independently
- **Full face animation pipeline in C++** — replaces `ABP_Face_PostProcess` entirely; `CopyPoseFromMesh → ControlRig → RigLogic` driven from a custom `FAnimInstanceProxy`
- **Procedural head look-at** — rate-limited angular slerp with 60° cone clamping, driven by a `USceneComponent` target, blended in component space on the worker thread
- **Event-driven greeting system** — Sarah greets → hospitality delay → NPC reacts with face animation + synchronized audio + head tracking; zero polling, zero tick
- **Idle crossfade** — three-idle cycle with loop detection and blend-weight interpolation (NPCDamsel subclass)
- **Automatic eye blinking** — timer-driven alpha blend feeding MetaHuman `CTRL_expressions_eyeBlinkL/R` curves directly

Every behaviour is driven by explicit function calls between components. There is no `Tick`, no `UAnimNotify`, no Blueprint graph node involved in any gameplay decision.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  AYoungWoman  (ANPCCharacter subclass)                          │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  Body Mesh   │  │  Face Mesh   │  │    Clothes Mesh      │  │
│  │ (GetMesh())  │  │  (FaceMesh)  │  │   (ClothesMesh)      │  │
│  │              │  │              │  │                      │  │
│  │ UYoungWoman  │  │ UYoungWoman  │  │  Attached to body    │  │
│  │ AnimInstance │  │ FaceAnim     │  │  (LeaderPose ready)  │  │
│  │              │  │              │  │                      │  │
│  │ FYoungWoman  │  │ FYoungWoman  │  │                      │  │
│  │ AnimProxy    │  │ FaceProxy    │  │                      │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
│                                                                  │
│  ┌───────────────────────┐  ┌────────────────────────────────┐  │
│  │ UYoungWomanActor      │  │ UYoungWomanFaceAnimActor       │  │
│  │ Component             │  │ Component                      │  │
│  │ (Body Brain)          │  │ (Face Brain)                   │  │
│  │                       │  │                                │  │
│  │  Idle state machine   │  │  Hard-loads anim assets        │  │
│  │  Greeting flow        │  │  Drives face proxy             │  │
│  │  Hospitality timer    │  │  Spawns audio                  │  │
│  │  LookAt on/off        │  │  Notifies body brain           │  │
│  └───────────────────────┘  └────────────────────────────────┘  │
│                                                                  │
│  USceneComponent  LookingPlace  ◄── world-space LookAt target   │
└─────────────────────────────────────────────────────────────────┘

Greeting event chain (zero tick, fully event-driven):

  Sarah presses G
      │
      ▼
  ASarahBase::TryInteract()
      │
      ▼
  UYoungWomanActorComponent::OnSarahGreeted(SarahHeadWorldLoc)
      │  positions LookingPlace immediately (alpha = 0)
      │  starts hospitality-delay FTimerHandle
      │
      ▼  [0.5 – 3.0 s delay depending on Hospitality enum]
      │
  UYoungWomanActorComponent::TriggerGreeting()
      │  sets HeadLookAtAlpha = 1  (proxy picks it up next frame)
      │
      ▼
  UYoungWomanFaceAnimActorComponent::PlayGreeting(index)
      │  drives FYoungWomanFaceProxy crossfade
      │  spawns UAudioComponent
      │
      ▼  [anim plays to end + 1 s hold]
      │
  UYoungWomanFaceAnim::FinishGreeting()
      │
      ▼
  UYoungWomanFaceAnimActorComponent::NotifyFaceAnimEnded()
      │
      ▼
  UYoungWomanActorComponent::NotifyGreetingEnded()
      │  sets HeadLookAtAlpha = 0
      │  resets LookingPlace
      │  state → Idle
```

---

## File Reference

### Data Assets

| File | Purpose |
|---|---|
| `YoungWomanAnimationDataAsset.h` | Primary data asset. Holds the `Greetings` array (`FYoungWomanGreeting` — anim + audio pairs), head bone config, LookAt tuning, blink timing, and hospitality enum. One asset per NPC variant. |
| `NPCDamselAnimationDataAsset.h` | Extends the primary asset with three idle sequences (`IdleA/B/C`) and crossfade speed. Used only by `ANPCDamsel`. |
| `YoungWomanDataAsset.h` | Personality asset (`EYoungWomanPersonality`). Controls how the NPC reacts at the dialogue/behaviour layer. Separate from the animation asset to allow mixing. |
| `YoungWomanInteractionsDataAsset.h` | Lightweight legacy asset for NPCs that only need greeting anim/audio arrays without the full pipeline. |

### Character Actor

| File | Purpose |
|---|---|
| `YoungWoman.h / .cpp` | Base NPC actor. Creates the three-mesh hierarchy, attaches both Actor Components, declares `LookingPlace` (`USceneComponent`), and exposes `StartLookAt()` / `StopLookAt()` / `MoveLookingPlaceTo()` as the only public interface the brain needs. Caches `HeadBoneIndex` once in `BeginPlay`. |
| `NPCDamsel.h / .cpp` | Minimal subclass. Only job: swap the body `AnimInstance` class to `UNPCDamselAnimInstance` in the constructor. No other logic. |

### Animation — Body Mesh

| File | Purpose |
|---|---|
| `YoungWomanAnimInstance.h / .cpp` | Game-thread anim instance. Smooths `HeadLookAtAlpha` via `FInterpConstantTo`, pushes `LookAtTargetWorld` / `LookAtAlpha` / `DeltaTime` to the proxy each frame. Also zeroes the shared `ABP_Body_PostProcess` "Head Movement Control" float via reflection so it cannot fight the C++ LookAt. |
| `NPCDamselAnimInstance.h / .cpp` | Derived anim instance. Overrides `CreateAnimInstanceProxy` to allocate `FNPCDamselIdleProxy` and store it in **both** `DamselProxy` (typed) and `MyProxy` (base-class upcast). Ticks the three-idle crossfade and detects loop boundaries by comparing `ActiveTime < PrevTime`. |

### Animation — Face Mesh

| File | Purpose |
|---|---|
| `YoungWomanFaceAnim.h / .cpp` | Game-thread anim instance for the face mesh. Manages the greeting crossfade state machine (playing → hold delay → finished), drives the blink timer loop, and syncs all variables to `FYoungWomanFaceProxy` each frame. Calls `FaceAC->NotifyFaceAnimEnded()` when finished. |

### Animation Proxies (Worker Thread)

| Proxy | What it does |
|---|---|
| `FYoungWomanAnimProxy` | Body proxy. `Evaluate` calls `FAnimInstanceProxy::Evaluate` then overlays `ApplyLookAt` if alpha > 0. `ApplyLookAt` works in component space: resolves `LookAtTargetWorld` → component space → angular-speed slerp → 60° cone clamp → `FQuat::FindBetweenNormals` → blended back to local space relative to parent. |
| `FYoungWomanFaceProxy` | Face proxy. Five-stage pipeline: `CopyPoseFromMesh` → face anim pose (greeting crossfade + blink curves) → merge → `ControlRig` (conditional) → `RigLogic` (conditional). Fully replaces `ABP_Face_PostProcess`. |
| `FNPCDamselIdleProxy` | Body proxy for Damsel. Inherits `FYoungWomanAnimProxy` to gain `ApplyLookAt()`. Adds the three-idle crossfade pose generation before calling the inherited look-at. |
| `FAnimNode_PosePassthrough` | Minimal helper struct. Injects a pre-computed `FPoseContext` into `FAnimNode_ControlRig` and `FAnimNode_RigLogic` source pins without Blueprint graph wiring. |

### Actor Components

| File | Purpose |
|---|---|
| `YoungWomanActorComponent.h / .cpp` | Body brain. Owns the greeting state machine (`EYoungWomanGreetingState`), idle rotation (`EYoungWomanIdleState`), and the `FTimerHandle` for the hospitality delay. Public API: `OnSarahGreeted`, `OnSarahLeft`, `NotifyIdleLooped`, `NotifyGreetingEnded`. Zero tick. |
| `YoungWomanFaceAnimActorComponent.h / .cpp` | Face brain. Hard-loads all `UAnimSequence` and `USoundBase` assets from the `Greetings` array synchronously in `BeginPlay` so `PlayGreeting` is zero-latency at runtime. Manages `UAudioComponent` lifetime. |
| `NPCDamselActorComponent.h` | Empty subclass of `UYoungWomanActorComponent`. Ready to hold Damsel-specific behaviour without touching the base class. |

### Player Character

| File | Purpose |
|---|---|
| `SarahBase.cpp` | Updated overlap handlers. `OnInteractionSphereBeginOverlap` casts to `AYoungWoman`; `OnSarahGreetingEnded` finds `UYoungWomanActorComponent` and calls `OnSarahGreeted` with Sarah's head bone world location. |

---

## Key Technical Decisions

### 1 — Custom FAnimInstanceProxy instead of AnimGraph

Every animation decision is made in C++ structs that run on the worker thread. There is no AnimGraph, no state machine node, no Blueprint variable driving any pose.

**Why:** AnimGraph Blueprint evaluation adds overhead and makes logic invisible to code review. More importantly, the three-mesh MetaHuman setup requires explicit control over which proxy drives which mesh and when — something AnimGraph cannot express cleanly across separate `USkeletalMeshComponent` instances.

### 2 — Proxy inheritance for shared LookAt

`FNPCDamselIdleProxy` inherits `FYoungWomanAnimProxy`. The base anim instance stores `MyProxy` as `FYoungWomanAnimProxy*`. In `CreateAnimInstanceProxy`, the Damsel allocates `FNPCDamselIdleProxy` and assigns it to **both** `DamselProxy` (for typed access) and `MyProxy` (for the base class update path).

This means `UYoungWomanAnimInstance::NativeUpdateAnimation` — which runs on every subclass via `Super::` — syncs `LookAtAlpha`, `LookAtTargetWorld`, and `CachedHeadBoneIndex` directly into the `FNPCDamselIdleProxy` object without any duplication or virtual dispatch overhead.

```
UYoungWomanAnimInstance::NativeUpdateAnimation()
    MyProxy->LookAtAlpha = CurrentLookAtAlpha;   // writes into FNPCDamselIdleProxy
    MyProxy->LookAtTargetWorld = LP->GetComponentLocation();

FNPCDamselIdleProxy::Evaluate()
    // ... idle crossfade pose ...
    if (LookAtAlpha > KINDA_SMALL_NUMBER)
        ApplyLookAt(Output);   // inherited from FYoungWomanAnimProxy
```

### 3 — Face pipeline replaces ABP_Face_PostProcess entirely

`FYoungWomanFaceProxy::Evaluate` implements the five nodes of the MetaHuman face post-process graph natively:

```
CopyPoseFromMesh   → copies body bone transforms (head, neck, spine) into face mesh space
EvaluateFaceAnimPose → samples greeting AnimSequence + injects blink curves
Merge              → body bones + face CTRL_* curves into one pose
ControlRig         → CR_MetaHuman_HeadMovement_IK_Proc (conditional on HeadControlSwitch curve)
RigLogic           → MetaHuman face deformation from CTRL_* curves
```

`FAnimNode_PosePassthrough` injects the pre-built `FPoseContext` into the `ControlRig` and `RigLogic` source pins without needing a connected AnimGraph link node.

**Why not use the Blueprint post-process ABP:** The Blueprint instance is shared across all MetaHumans of the same skeleton. It cannot be customized per NPC instance without spawning a new class. Owning the proxy means the greeting crossfade, blink state, and RigLogic toggle are all per-instance with no Blueprint overhead.

### 4 — LookAt in component space with angular-speed clamping

Head rotation is computed entirely in component space on the worker thread:

```
1. Transform LookAtTargetWorld → component space
2. Compute desired look direction from head bone location
3. Clamp to 60° cone from the bone's current rest forward vector
4. Angular-speed slerp: advance SmoothedLookDirCS by at most
   (LookAtTrackingSpeedRadPerSec × DeltaTime) radians per frame
5. FQuat::FindBetweenNormals(currentAxis, smoothedDir) → delta rotation
6. Slerp final rotation by LookAtAlpha (0..1) for soft enable/disable
7. Write back to local space relative to parent bone
```

The 60° cone prevents the head from snapping to impossible angles if Sarah walks behind the NPC. The tracking speed cap (default 360°/s, tunable in the data asset) ensures the motion reads as a natural human head turn rather than an instant snap.

### 5 — Zero-tick event-driven architecture

No component has `PrimaryActorTick.bCanEverTick = true`. State changes happen through three explicit call paths:

- **Overlap enter/exit** → `OnSarahGreeted` / `OnSarahLeft`
- **FTimerHandle** → `TriggerGreeting` (hospitality delay)
- **Anim instance callbacks** → `NotifyFaceAnimEnded` → `NotifyGreetingEnded`

This means the entire NPC system costs zero CPU when nothing is happening, scales to any number of idle NPCs, and is trivially debuggable — every state change is a named function call with a `UE_LOG` entry.

### 6 — Hard-loading assets in BeginPlay

`UYoungWomanFaceAnimActorComponent::BeginPlay` calls `LoadSynchronous()` on every soft pointer in the `Greetings` array. The assets are stored in `TArray<TObjectPtr<UAnimSequence>>` and `TArray<TObjectPtr<USoundBase>>`.

**Why:** The greeting fires after a 0.5–3 s human-perceived delay. Any async load that hasn't completed by then would either play nothing or hitch. Loading once at spawn amortises the cost across the level load where frame hitches are acceptable.

---

## System Walkthrough

### Adding a new YoungWoman NPC

1. Place `AYoungWoman` (or a Blueprint subclass) in the level.
2. Assign a `UYoungWomanAnimationDataAsset` to the `AnimDataAsset` slot.
3. Fill `Greetings` with `FYoungWomanGreeting` entries (soft refs to `UAnimSequence` + `USoundBase`).
4. Optionally assign a `UYoungWomanDataAsset` for personality.
5. Set `Hospitality` on the animation data asset to control reply delay.
6. Assign body, face, and clothes skeletal meshes to the three mesh components.

That is the complete setup. No Blueprint logic required.

### Adding a new NPCDamsel NPC

1. All steps above, but place `ANPCDamsel` instead.
2. Assign a `UNPCDamselAnimationDataAsset` (extends the base) to `AnimDataAsset`.
3. Fill `IdleA`, `IdleB`, `IdleC` with three idle `UAnimSequence` assets.
4. Set `IdleBlendInterpSpeed` (default 4.0) to taste.

The damsel will automatically cycle A → B → C → A, blending between idles on each loop boundary.

### Extending with a new NPC subtype

```cpp
// Header
UCLASS()
class GAME_API AMyNewNPC : public AYoungWoman
{
    GENERATED_BODY()
public:
    AMyNewNPC();
};

// Source — override only what differs
AMyNewNPC::AMyNewNPC()
{
    GetMesh()->SetAnimInstanceClass(UMyNewNPCAnimInstance::StaticClass());
}
```

The body brain, face brain, LookAt system, and greeting flow are all inherited automatically.

---

## Data Asset Configuration

### UYoungWomanAnimationDataAsset

| Property | Type | Default | Description |
|---|---|---|---|
| `Greetings` | `TArray<FYoungWomanGreeting>` | Empty | Array of anim + audio pairs. One is picked at random on each greeting. |
| `Hospitality` | `EYoungWomanHospitality` | Friendly | Controls reply delay: Friendly 0.5–1 s, Slower 1–2 s, LessFriendly 2–3 s. |
| `HeadBoneName` | `FName` | `"head"` | Body skeleton head bone. Used for LookAt pivot. |
| `LookAtAxis` | `EYoungWomanBoneAxis` | Y | Forward axis of the head bone. MetaHuman default is Y. |
| `HeadLookAtTrackingSpeed` | `float` | 360 °/s | Max angular speed of the head turn. Lower = lazier turn. |
| `LookAtAlphaBlendSpeed` | `float` | 3.0 | Speed of the 0→1 alpha blend when LookAt activates. |
| `LookAtEyeHeightOffset` | `float` | 10 cm | Z offset added to Sarah's head location so the NPC looks at eye level. |
| `FaceBlendInterpSpeed` | `float` | 4.0 | Crossfade speed between greeting animations. |
| `BlinkClosedDurationMin/Max` | `float` | 0.10 / 0.25 s | Random range for how long eyes stay closed. |
| `BlinkOpenDurationMin/Max` | `float` | 2.0 / 4.0 s | Random range between blinks. |
| `BlinkAlphaBlendSpeed` | `float` | 10.0 | Speed of the eyelid close/open alpha blend. |
| `EyeBlinkLeftCurveName` | `FName` | `CTRL_expressions_eyeBlinkL` | MetaHuman curve name for left eyelid. |
| `EyeBlinkRightCurveName` | `FName` | `CTRL_expressions_eyeBlinkR` | MetaHuman curve name for right eyelid. |

### UNPCDamselAnimationDataAsset (extends above)

| Property | Type | Default | Description |
|---|---|---|---|
| `IdleA / IdleB / IdleC` | `TObjectPtr<UAnimSequence>` | None | Three idle animations cycled in order on each loop. |
| `IdleBlendInterpSpeed` | `float` | 4.0 | Crossfade speed between idle A/B/C. |

---

## Integration Guide

### Required includes in your module's `Build.cs`

```csharp
PrivateDependencyModuleNames.AddRange(new string[]
{
    "RigLogicModule",   // FAnimNode_RigLogic
    "ControlRig",       // FAnimNode_ControlRig
    "AnimGraphRuntime", // FAnimNode_CopyPoseFromMesh, FAnimationRuntime
    "AnimationCore",
});
```

### Folder layout expected by include paths

```
Source/
└── YourGame/
    ├── Character/
    │   ├── NPC/
    │   │   └── YoungWoman/
    │   │       ├── YoungWoman.h / .cpp
    │   │       ├── YoungWomanAnimInstance.h / .cpp
    │   │       ├── YoungWomanFaceAnim.h / .cpp
    │   │       ├── YoungWomanActorComponent.h / .cpp
    │   │       ├── YoungWomanFaceAnimActorComponent.h / .cpp
    │   │       ├── YoungWomanDataAsset.h
    │   │       └── NPCDamsel/
    │   │           ├── NPCDamsel.h / .cpp
    │   │           ├── NPCDamselAnimInstance.h / .cpp
    │   │           └── NPCDamselActorComponent.h
    │   └── Sarah/
    │       └── SarahBase.cpp
    └── Data/
        └── DataAssets/
            └── Animation/
                └── NPC/
                    └── YoungWoman/
                        ├── YoungWomanAnimationDataAsset.h
                        ├── NPCDamselAnimationDataAsset.h
                        └── YoungWomanInteractionsDataAsset.h
```

### One required update outside this module

`SarahBase.h` must rename the `TWeakObjectPtr<AHumanElf> CurrentHumanElf` member to `TWeakObjectPtr<AYoungWoman> CurrentYoungWoman` to match `SarahBase.cpp`.

---

## Engine & Dependencies

| Requirement | Version |
|---|---|
| Unreal Engine | 5.5 + (tested on 5.8) |
| MetaHuman Plugin | Required for `FAnimNode_RigLogic` and `CR_MetaHuman_HeadMovement_IK_Proc` |
| ControlRig Plugin | Required for `FAnimNode_ControlRig` |
| C++ Standard | C++17 (UE default) |
| Blueprint gameplay logic | None — zero dependency |

---

## About

This system is part of **Moon** — an open-world narrative RPG set in Crusades-era Beirut developed solo under **Strawberries Studio**.

The YoungWoman NPC module was designed with three constraints:

- Every gameplay decision must be readable, debuggable, and modifiable in C++ without opening the editor
- Zero per-frame cost when no interaction is active
- The MetaHuman face pipeline must be owned per-NPC instance, not shared

These constraints led directly to the custom proxy architecture, the brain/AC split, and the event-driven call chain documented above.
