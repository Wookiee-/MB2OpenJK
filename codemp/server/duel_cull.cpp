#include "server.h"
#include "duel_cull.h"

/*
================
DuelCull

Returns 0 for standard solid collision.
Returns 2 to bypass collision (Ghosting).
================
*/
int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch) {
    // 1. Instant Exit (Fastest check first)
    // Always check the cvar first to avoid logic overhead if disabled
    if (!sv_snapShotDuelCull || !sv_snapShotDuelCull->integer) return 0;
    if (ent == touch) return 0;

    // 2. Direct State Access
    // We cast to playerState_t because the grep confirms this is the source of truth
    playerState_t *entPs = (playerState_t *)ent->playerState;
    playerState_t *touchPs = (playerState_t *)touch->playerState;

    // 3. Safety Check
    // Critical: ent->playerState will be NULL for map objects and non-player entities
    if (!entPs || !touchPs) {
        return 0;
    }

    // 4. Local Cache Logic
    // Accessing the struct members once and storing them in local variables 
    // reduces the number of memory lookups the 3700X has to perform.
    int entDuel = entPs->duelInProgress;
    int touchDuel = touchPs->duelInProgress;

    // 5. Duel Resolution
    if (entDuel && touchDuel) {
        // If they are dueling each other, they remain SOLID
        if (entPs->duelIndex == touch->s.number) {
            return 0; 
        }
        // Dueling different people: GHOST
        return 2; 
    }

    // If one is in a duel and the other isn't: GHOST
    // This stops bystanders from being "glue" for duelists
    if (entDuel || touchDuel) {
        return 2;
    }

    return 0;
}