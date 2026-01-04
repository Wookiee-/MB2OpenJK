#include "qcommon/qcommon.h"
#include "duel_cull.h"

static qboolean isPlayer(sharedEntity_t *ent) {
	if (ent->s.eType == ET_PLAYER)
		return qtrue;

	return qfalse;
}

static qboolean isNPC(sharedEntity_t *ent) {
	if (ent->s.eType == ET_NPC)
		return qtrue;

	return qfalse;
}

static qboolean isMover(sharedEntity_t *ent) {
	if (ent->s.eType == ET_MOVER)
		return qtrue;

	return qfalse;
}

sharedEntity_t *flatten(sharedEntity_t *ent) {
    if (ent->s.eType == ET_MISSILE) {
        return SV_GentityNum(ent->r.ownerNum);
    }

    if (ent->s.eType >= ET_EVENTS) {
        return ent; 
    }

    if (ent->s.eFlags & EF_PLAYER_EVENT) {
        return ent; 
    }

    if ((ent->s.event & ~EV_EVENT_BITS) == EV_GRENADE_BOUNCE) {
        return ent; 
    }

    return ent;
}

static playerState_t *GetPS(sharedEntity_t *ent) {
	return SV_GameClientNum(ent->s.number);
}

static qboolean isDueling(sharedEntity_t *ent) {
	if (isPlayer(flatten(ent)) && GetPS(flatten(ent))->duelInProgress)
		return qtrue;

	return qfalse;
}

static qboolean isActor(sharedEntity_t *ent) {
	if (!isMover(ent) && (isPlayer(flatten(ent)) || isNPC(flatten(ent))))
		return qtrue;

	return qfalse;
}

static qboolean isDuelOpponent(sharedEntity_t *A, sharedEntity_t *B) { //wtf void??
	auto a = flatten(A);
	auto b = flatten(B);

	if (isDueling(a) && isDueling(b) && (a == b || a->playerState->duelIndex == SV_NumForGentity(b)))
		return qtrue;

	return qfalse;
}

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch) {
	// The original cvar check remains the entry point.
	if (!sv_snapShotDuelCull->integer)
		return 0;

	auto culledTouch = flatten(touch);

	if (!isActor(ent)) {
		return 0;
	}

	// --- 2. Refining Actor Culling Logic / Duelist Culling ---
	if (isDueling(ent)) {
		if (isDuelOpponent(ent, culledTouch)) {
			return 0; // Don't cull.
		}
		if (isActor(culledTouch)) {
			return 1; // Cull other actors and their direct events.
		}
		if (touch->r.ownerNum != ENTITYNUM_NONE) {
			sharedEntity_t *owner = SV_GentityNum(touch->r.ownerNum);

			if (isActor(owner) && !isDuelOpponent(ent, owner)) {
				return 1;
			}
		}

		return 0;
	}


	if (isActor(culledTouch)) {
		if (isDueling(culledTouch)) {
			return 2; // Don't cull and don't clip (spectating).
		}
		return 0; // Don't cull.
	}

	return 0;
}
