#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/YoungWoman/YoungWoman.h"
#include "NPCDamsel.generated.h"

/**
 * ANPCDamsel — stands in place cycling three idles.
 * Character is a pure placeholder; the body anim instance + both ACs do all work.
 */
UCLASS()
class GAME_API ANPCDamsel : public AYoungWoman
{
    GENERATED_BODY()

public:
    ANPCDamsel();
};
