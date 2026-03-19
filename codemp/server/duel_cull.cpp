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
    // 1. Master Switch: If cvar is 0, behave exactly like vanilla OpenJK
    if (!sv_snapShotDuelCull || sv_snapShotDuelCull->integer == 0) {
        return 0;
    }

    // 2. Identity: Don't check collision against yourself
    if (ent == touch) {
        return 0;
    }

    // 3. Safety: Only apply duel logic if both entities are players (0 to sv_maxclients-1)
    // This prevents ghosting through walls, floors, or projectiles.
    if (ent->s.number < 0 || ent->s.number >= sv_maxclients->integer || 
        touch->s.number < 0 || touch->s.number >= sv_maxclients->integer) {
        return 0;
    }

    // 4. State Access: Get playerState through the gentity pointer
    if (!svs.clients[ent->s.number].gentity || !svs.clients[touch->s.number].gentity) {
        return 0;
    }

    playerState_t *entPs = svs.clients[ent->s.number].gentity->playerState;
    playerState_t *touchPs = svs.clients[touch->s.number].gentity->playerState;

    if (!entPs || !touchPs) {
        return 0;
    }

    // 5. Duel Logic:
    // If either player is not dueling, or they are dueling each other, remain SOLID.
    if (!entPs->duelInProgress || !touchPs->duelInProgress) {
        return 0; 
    }

    if (entPs->duelIndex == touch->s.number && touchPs->duelIndex == ent->s.number) {
        return 0; 
    }

    // 6. The "Cull" Case: 
    // One or both are in a duel, but not with each other. Pass through.
    return 2; 
}
