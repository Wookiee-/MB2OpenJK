#include "qcommon/qcommon.h"
#include "server.h"         
#include "duel_cull.h"      
#include "sv_gameapi.h"

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch, moveclip_t *clip) {
    if (!sv_snapShotDuelCull->integer) return 0;

    // Standard Boundary Box Safety
    if (clip && (clip->contentmask & (MASK_SHOT | CONTENTS_BODY))) {
        return 0; 
    }

    // Direct access to the new struct member
    if (ent->s_duelMask == 0 && touch->s_duelMask == 0) {
        return 0;
    }

    // DUELIST PERSPECTIVE
    if (ent->s_duelMask == 1) {
        if (touch->s_duelMask == 2) return 0; // Solid vs Opponent
        return 2; // Ghost others
    }

    // BYSTANDER PERSPECTIVE
    if (touch->s_duelMask != 0) {
        return 2; // Ghost duelists
    }

    return 0;
}