#include "qcommon/qcommon.h"
#include "duel_cull.h"
#include "sv_gameapi.h"

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch, playerState_t *ps) {
    // 1. PERFORMANCE GATE: Exit immediately if feature is off.
    // This is the most efficient way to handle "Stock" performance.
    if (!sv_snapShotDuelCull->integer) {
        return 0;
    }

    // 2. TYPE FILTER: Only process Players and NPCs.
    if (touch->s.eType != ET_PLAYER && touch->s.eType != ET_NPC) {
        return 0; 
    }

    // 3. NPC SAFETY: NPCs are always solid.
    if (touch->s.eType == ET_NPC) {
        return 0; 
    }

    // We use the 'ps' passed from the engine (sv_snapshot.cpp)
    int touchNum = touch->s.number;

    // 4. DUELIST LOGIC: If 'ps' shows a duel, ghost everyone except opponent.
    if (ps && ps->duelInProgress) {
        if (ps->duelIndex != touchNum) {
            return 2; // Ghost bystander
        }
        return 0; // Solid opponent
    }

    // 5. BYSTANDER LOGIC: Ghost players who are currently in a duel.
    if (touch->s.eType == ET_PLAYER) {
        // We only perform the lookup for the OTHER player.
        playerState_t *ops = SV_GameClientNum(touchNum);
        if (ops && ops->duelInProgress) {
            return 2; 
        }
    }

    return 0;
}