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

    // 4. State Access: Use the pointers passed directly to the function
    // This is faster and avoids the 'Slot 15' sync issue
    playerState_t *entPs = (playerState_t *)ent->playerState;
    playerState_t *touchPs = (playerState_t *)touch->playerState;

    if (!entPs || !touchPs) {
        return 0;
    }

    // 5. Duel Logic:
    // Check if BOTH players are in a duel. 
    // If one is a bystander (duelInProgress == 0), they should GHOST through duelists.
    
    // If both are dueling each other, they stay SOLID.
    if (entPs->duelInProgress && touchPs->duelInProgress) {
        if (entPs->duelIndex == touch->s.number && touchPs->duelIndex == ent->s.number) {
            return 0; 
        }
        return 2; // They are dueling, but not each other. Ghost.
    }

    // If one is dueling and the other isn't, they GHOST.
    if (entPs->duelInProgress || touchPs->duelInProgress) {
        return 2;
    }

    return 0;
}
