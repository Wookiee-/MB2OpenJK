#include "server.h"
#include "duel_cull.h"

#include "server.h"
#include "duel_cull.h"

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch) {
    // 1. The Master Switch
    if (sv_snapShotDuelCull->integer == 0) {
        return 0;
    }

    // 2. Standard Boundary Check
    if (ent->s.number < 0 || ent->s.number >= sv_maxclients->integer || 
        touch->s.number < 0 || touch->s.number >= sv_maxclients->integer) {
        return 0;
    }

    // 3. Get the Player States via the gentity pointer
    // This is the most compatible way across different engine branches
    if (!svs.clients[ent->s.number].gentity || !svs.clients[touch->s.number].gentity) {
        return 0;
    }

    playerState_t *entPs = svs.clients[ent->s.number].gentity->playerState;
    playerState_t *touchPs = svs.clients[touch->s.number].gentity->playerState;

    if (!entPs || !touchPs) {
        return 0;
    }

    // 4. Simplest possible Duel Logic
    // If not in a duel, or dueling each other -> Solid (0)
    // Otherwise -> Ghost (2)
    
    if (!entPs->duelInProgress || !touchPs->duelInProgress) {
        return 0; 
    }

    if (entPs->duelIndex == touch->s.number && touchPs->duelIndex == ent->s.number) {
        return 0; 
    }

    // This is the ONLY case where we change behavior from "Off"
    return 2; 
}