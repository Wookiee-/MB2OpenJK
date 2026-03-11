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

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch, playerState_t *ps) {
    // --- 1. PERFORMANCE GATE ---
    // If the feature is off, exit immediately to save CPU for all players.
    if (!sv_snapShotDuelCull->integer) {
        return 0;
    }

    int entNum = ent->s.number;

    // --- 2. LOGGING MODIFICATIONS (State-Locked) ---
    // We use the 'ps' passed from the engine instead of calling GetPS(ent).
    if (entNum >= 0 && entNum < MAX_CLIENTS && ps && isPlayer(ent)) {
        qboolean isCurrentlyDueling = ps->duelInProgress ? qtrue : qfalse;

        // START TRIGGER: Log when the duel begins
        if (isCurrentlyDueling && !oldDuelState[entNum]) {
            int myOpponentIdx = ps->duelIndex; 

            if (myOpponentIdx >= 0 && myOpponentIdx < MAX_CLIENTS && myOpponentIdx != entNum) {
                // Fetch opponent's state once using the index
                playerState_t *oppPs = SV_GameClientNum(myOpponentIdx);

                // RECIPROCITY CHECK: Ensure both are locked to each other
                if (oppPs && oppPs->duelInProgress && oppPs->duelIndex == entNum) {
                    // LOCK: This prevents any more "Start" logs until the duel ends
                    oldDuelState[entNum] = qtrue;
                    duelOpponent[entNum] = myOpponentIdx;

                    // ID FILTER: Only one side prints to avoid double-logging
                    if (entNum < myOpponentIdx) {
                        char p1Name[MAX_NETNAME], p2Name[MAX_NETNAME];
                        GetPlayerName(entNum, p1Name, sizeof(p1Name));
                        GetPlayerName(myOpponentIdx, p2Name, sizeof(p2Name));
                        GVM_LogPrintf("DuelStart: %s challenged %s to a private duel\n", p1Name, p2Name);
                    }
                }
            }
        }
        // END TRIGGER: Log when the duel ends
        else if (!isCurrentlyDueling && oldDuelState[entNum]) {
            // MB2 Loser is set to 1 HP. Winner is > 1.
            if (ps->stats[STAT_HEALTH] > 1) {
                int loserIdx = duelOpponent[entNum];
                
                if (loserIdx >= 0 && loserIdx < MAX_CLIENTS) {
                    char winnerName[MAX_NETNAME], loserName[MAX_NETNAME];
                    GetPlayerName(entNum, winnerName, sizeof(winnerName));
                    GetPlayerName(loserIdx, loserName, sizeof(loserName));

                    GVM_LogPrintf("DuelEnd: %s has defeated %s in a private duel\n", winnerName, loserName);
                }
            }

            // UNLOCK: Reset trackers so they can duel again
            oldDuelState[entNum] = qfalse;
            duelOpponent[entNum] = -1; 
        }
    }

    // --- 3. TYPE & SAFETY CHECKS ---
    // Handle Thrown Sabers: Fixes the 'return on block' bug
    if (touch->s.eType == ET_MISSILE) {
        if (ps && ps->duelInProgress && touch->s.otherEntityNum != ps->duelIndex) {
            return 2; // Ghost the saber for anyone NOT in the duel
        }
        return 0; // Keep saber solid for the opponent and regular play
    }

    // Safety Net: Map objects (doors, floors, triggers) are always solid
    if (touch->s.eType != ET_PLAYER && touch->s.eType != ET_NPC) {
        return 0; 
    }

    int touchNum = touch->s.number;

    // --- 4. CULLING LOGIC ---

    // NPC/DUMMY LOGIC:
    if (touch->s.eType == ET_NPC) {
        // If the viewer is dueling, HIDE the dummy to clear the arena
        if (ps && ps->duelInProgress) {
            return 1; 
        }
        // For bystanders, the dummy is solid and visible
        return 0; 
    }

    // DUELIST LOGIC: If viewer is dueling, ghost everyone except their opponent
    if (ps && ps->duelInProgress) {
        if (ps->duelIndex != touchNum) {
            return 2; // Ghost bystander
        }
        return 0; // Solid opponent
    }

    // BYSTANDER LOGIC: Ghost players who are currently dueling
    if (touch->s.eType == ET_PLAYER) {
        playerState_t *ops = SV_GameClientNum(touchNum);
        if (ops && ops->duelInProgress) {
            return 2; 
        }
    }

    return 0;
}