#include "Character/NPC/YoungWoman/NPCDamsel/NPCDamsel.h"
#include "Character/NPC/YoungWoman/NPCDamsel/NPCDamselAnimInstance.h"

ANPCDamsel::ANPCDamsel()
{
    // Override the base body anim instance with the idle-cycling version
    GetMesh()->SetAnimInstanceClass(UNPCDamselAnimInstance::StaticClass());
}
