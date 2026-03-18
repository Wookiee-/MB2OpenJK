#include "qcommon/qcommon.h"
#include "server.h"         
#include "duel_cull.h"      
#include "sv_gameapi.h"

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch, moveclip_t *clip) {
    if (!sv_snapShotDuelCull->integer) return 0;

    // 1. Standard Boundary Box Safety
    // Sabers, shots, and body-impact traces are ALWAYS solid.
    if (clip && (clip->contentmask & (MASK_SHOT | CONTENTS_BODY))) {
        return 0; 
    }

    // 2. NEUTRAL CASE: Both are bystanders
    // Neither in a duel? Stay solid (Standard Box).
    if (ent->s_duelMask == 0 && touch->s_duelMask == 0) {
        return 0;
    }

    // 3. DUEL CASE: Initiator vs. Opponent
    // If we are a matched pair, stay solid to each other.
    if (ent->s_duelMask == 1 && touch->s_duelMask == 2) return 0;
    if (ent->s_duelMask == 2 && touch->s_duelMask == 1) return 0;

    // 4. CULL CASE: Anything else involving a duelist
    // If one person is in a duel and the other isn't (or is in a DIFFERENT duel),
    // we return 2 to ghost them.
    if (ent->s_duelMask != 0 || touch->s_duelMask != 0) {
        return 2; 
    }

    return 0;
}