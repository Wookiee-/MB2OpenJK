#include "qcommon/qcommon.h"
#include "duel_cull.h"
#include "sv_gameapi.h"

static playerState_t *GetPS(sharedEntity_t *ent) {
	return SV_GameClientNum(ent->s.number);
}

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch) {
    // --- 1. INITIAL CHECKS ---
    // If culling is disabled via cvar, everything stays solid.
    if (!sv_snapShotDuelCull->integer) {
        return 0;
    }

    // Only process Players and NPCs; everything else (missiles, doors, items) stays solid.
    if (touch->s.eType != ET_PLAYER && touch->s.eType != ET_NPC) {
        return 0; 
    }

    // --- 2. NPC SAFETY FIX ---
    // If the entity being touched is an NPC (like a training dummy), always return 0 (Solid).
    // This fixes the bug where bystanders could phase through or get stuck in NPCs.
    if (touch->s.eType == ET_NPC) {
        return 0; 
    }

    playerState_t *ps = GetPS(ent);
    int touchNum = touch->s.number;

    // --- 3. DUELIST LOGIC ---
    // If the player calling the check is in a duel, ghost everyone except their opponent.
    if (ps && ps->duelInProgress) {
        if (ps->duelIndex != touchNum) {
            return 2; // Visible Ghost (No physical collision, but still rendered)
        }
        return 0; // Solid (Maintain collision with duel opponent)
    }

    // --- 4. BYSTANDER LOGIC ---
    // If the player is not in a duel, check if the person they are touching is in one.
    if (touch->s.eType == ET_PLAYER) {
        playerState_t *ops = GetPS(touch);
        if (ops && ops->duelInProgress) {
            return 2; // Visible Ghost (Pass through active duelists)
        }
    }

    return 0; // Default: Solid
}