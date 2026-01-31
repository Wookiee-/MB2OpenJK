#include "qcommon/qcommon.h"
#include "duel_cull.h"
#include "sv_gameapi.h"

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

	// 1. Try to get the name from Configstrings (The standard way)
	SV_GetConfigstring(CS_PLAYERS + clientNum, configstring, sizeof(configstring));
	value = Info_ValueForKey(configstring, "n");

	// 2. If configstring is empty or generic, check the engine's direct client record
	// svs.clients is globally available via server.h
	if (!value || !value[0]) {
		client_t *cl = &svs.clients[clientNum];
		if (cl && cl->name[0]) {
			value = cl->name;
		}
	}

	if (value && value[0]) {
		char cleanName[MAX_NETNAME];
		Q_strncpyz(cleanName, value, sizeof(cleanName));
		
		// 3. Strip color codes (e.g., ^1, ^7) for the log file
		Q_CleanStr(cleanName); 
		
		Q_strncpyz(outName, cleanName, maxSize);
	} else {
		// Final fallback if absolutely no name is found
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
            // Validate the opponent exists and is actually the person we are dueling
            int targetIdx = ps->duelIndex;
            if (targetIdx >= 0 && targetIdx < MAX_CLIENTS && targetIdx != entNum) {
                
                // CRITICAL FILTER: Only print if this player's ID is lower than the target
                // This prevents a 3rd party or the opponent from double-printing
                if (entNum < targetIdx) {
                    char p1Name[MAX_NETNAME], p2Name[MAX_NETNAME];
                    GetPlayerName(entNum, p1Name, sizeof(p1Name));
                    GetPlayerName(targetIdx, p2Name, sizeof(p2Name));

                    GVM_LogPrintf("DuelStart: %s challenged %s to a private duel\n", p1Name, p2Name);
                }
                
                duelOpponent[entNum] = targetIdx;
                oldDuelState[entNum] = qtrue;
            }
        }

        // END TRIGGER: Log the outcome when the duel ends
        else if (!isCurrentlyDueling && oldDuelState[entNum]) {
            // Only the winner prints the log. 
            // We check health > 1 because MB2 sets the loser to exactly 1 HP.
            if (ps && ps->stats[STAT_HEALTH] > 1) {
                int loserIdx = duelOpponent[entNum];
                
                // Double-check the opponent index still makes sense
                if (loserIdx >= 0 && loserIdx < MAX_CLIENTS) {
                    char winnerName[MAX_NETNAME], loserName[MAX_NETNAME];
                    
                    GetPlayerName(entNum, winnerName, sizeof(winnerName));
                    GetPlayerName(loserIdx, loserName, sizeof(loserName));

                    GVM_LogPrintf("DuelEnd: %s has defeated %s in a private duel\n", winnerName, loserName);
                }
            }

            // Always reset for both players
            oldDuelState[entNum] = qfalse;
            duelOpponent[entNum] = 0;
        }
    }
    
    // --- 2. ORIGINAL CULLING LOGIC ---
    if (!sv_snapShotDuelCull->integer)
        return 0;

    if (touch->s.eType != ET_PLAYER && touch->s.eType != ET_NPC) {
        return 0; 
    }

    auto culledTouch = flatten(touch);

    if (!isActor(ent)) {
        return 0;
    }

    if (isDueling(ent)) {
        if (isDuelOpponent(ent, culledTouch)) {
            return 0; 
        }
        if (isActor(culledTouch)) {
            return 1; 
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
            return 2; 
        }
        return 0; 
    }

    return 0;
}