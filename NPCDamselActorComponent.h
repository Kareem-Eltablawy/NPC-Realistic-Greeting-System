#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/YoungWoman/YoungWomanActorComponent.h"
#include "NPCDamselActorComponent.generated.h"

/**
 * UNPCDamselActorComponent
 *
 * Body brain for ANPCDamsel NPCs.
 * Inherits all greeting, look-at, and idle-state logic from
 * UYoungWomanActorComponent.  Extend here for Damsel-specific behaviour.
 */
UCLASS(ClassGroup = (YoungWoman), meta = (BlueprintSpawnableComponent))
class GAME_API UNPCDamselActorComponent : public UYoungWomanActorComponent
{
    GENERATED_BODY()
};
