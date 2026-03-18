#include "qcommon/qcommon.h"
#include "server.h"         
#include "duel_cull.h"      
#include "sv_gameapi.h"

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch, moveclip_t *clip) {
    if (!sv_snapShotDuelCull->integer) return 0;

    // Safety: Only cull for actual clients (0-31)
    if (ent->s.number >= MAX_CLIENTS || touch->s.number >= MAX_CLIENTS) {
        return 0;
    }

    int entOpponent = sv_duelTable[ent->s.number];
    int touchOpponent = sv_duelTable[touch->s.number];

    // CASE 1: Neither is in a duel -> Both are visible
    if (entOpponent == -1 && touchOpponent == -1) {
        return 0;
    }

    // CASE 2: They are dueling each other -> Both are visible to each other
    if (entOpponent == touch->s.number && touchOpponent == ent->s.number) {
        return 0;
    }

    // CASE 3: One (or both) are in a duel, but not with each other
    // Result: Return 2 (Ghost/Cull)
    return 2; 
}