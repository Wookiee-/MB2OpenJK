#include "qcommon/qcommon.h"
#include "duel_cull.h"

// Persistent state trackers
static qboolean oldDuelState[MAX_CLIENTS] = { qfalse };
static int      duelOpponent[MAX_CLIENTS] = { 0 };

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

// Helper to extract clean names
static void GetPlayerName(int clientNum, char *outName, int maxSize) {
    char configstring[MAX_CONFIGSTRINGS];
    const char *value;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
        Q_strncpyz(outName, "Unknown", maxSize);
        return;
    }
    SV_GetConfigstring(CS_PLAYERS + clientNum, configstring, sizeof(configstring));
    value = Info_ValueForKey(configstring, "n");
    if (value && value[0]) {
        Q_strncpyz(outName, value, maxSize);
    } else {
        Com_sprintf(outName, maxSize, "Player %d", clientNum);
    }
}

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch) {
    int entNum = ent->s.number;

    // --- 1. LOGGING MODIFICATIONS ---
    if (entNum >= 0 && entNum < MAX_CLIENTS && isPlayer(ent)) {
        playerState_t *ps = GetPS(ent);
        qboolean isCurrentlyDueling = (ps && ps->duelInProgress) ? qtrue : qfalse;

        // START TRIGGER: Log when the duel begins
        if (isCurrentlyDueling && !oldDuelState[entNum]) {
            // Validate the opponent exists and isn't self
            if (ps->duelIndex >= 0 && ps->duelIndex < MAX_CLIENTS && ps->duelIndex != entNum) {
                
                // Only log from the lower Client ID to prevent double-printing the start
                if (entNum < ps->duelIndex) {
                    char p1Name[MAX_NETNAME], p2Name[MAX_NETNAME];
                    GetPlayerName(entNum, p1Name, sizeof(p1Name));
                    GetPlayerName(ps->duelIndex, p2Name, sizeof(p2Name));

                    Com_Printf("DUEL_START: %s challenged %s to a private duel\n", p1Name, p2Name);
                }
                
                duelOpponent[entNum] = ps->duelIndex;
                oldDuelState[entNum] = qtrue;
            }
        }

        // END TRIGGER: Log the outcome when the duel ends
        else if (!isCurrentlyDueling && oldDuelState[entNum]) {
            // Only the winner logs the 'defeated' message to keep it to one line
            // In MB2 private duels, losers are set to 1 HP.
            if (ps && ps->stats[STAT_HEALTH] > 1) {
                char winnerName[MAX_NETNAME], loserName[MAX_NETNAME];
                
                GetPlayerName(entNum, winnerName, sizeof(winnerName));
                GetPlayerName(duelOpponent[entNum], loserName, sizeof(loserName));

                Com_Printf("DUEL_FINISH: %s has defeated %s in a private duel\n", winnerName, loserName);
            }

            // Cleanup state for the next duel
            oldDuelState[entNum] = qfalse;
            duelOpponent[entNum] = 0;
        }
    }

	// The original cvar check remains the entry point.
	if (!sv_snapShotDuelCull->integer)
		return 0;

	if (touch->s.eType != ET_PLAYER && touch->s.eType != ET_NPC) {
        return 0; 
    }

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
