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

    // --- 1. LOGGING MODIFICATIONS (State-Locked) ---
    if (entNum >= 0 && entNum < MAX_CLIENTS && isPlayer(ent)) {
        playerState_t *ps = GetPS(ent);
        qboolean isCurrentlyDueling = (ps && ps->duelInProgress) ? qtrue : qfalse;

        // START TRIGGER: Log when the duel begins
        if (isCurrentlyDueling && !oldDuelState[entNum]) {
            int myOpponentIdx = ps->duelIndex; 

            if (myOpponentIdx >= 0 && myOpponentIdx < MAX_CLIENTS && myOpponentIdx != entNum) {
                // THE HANDSHAKE: Get the player state of the opponent
                sharedEntity_t *oppEnt = SV_GentityNum(myOpponentIdx);
                playerState_t *oppPs = GetPS(oppEnt);

                // RECIPROCITY CHECK: Ensure both are locked to each other
                if (oppPs && oppPs->duelInProgress && oppPs->duelIndex == entNum) {
                    // LOCK: This prevents any more "Start" logs until the duel ends
                    oldDuelState[entNum] = qtrue;
                    duelOpponent[entNum] = myOpponentIdx;

                    // ID FILTER: Only one side prints to avoid double-logging the pair
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
            if (ps && ps->stats[STAT_HEALTH] > 1) {
                int loserIdx = duelOpponent[entNum];
                
                // We use the ID we "Locked" at the start, because the engine
                // has likely already cleared ps->duelIndex by this frame.
                if (loserIdx >= 0 && loserIdx < MAX_CLIENTS) {
                    char winnerName[MAX_NETNAME], loserName[MAX_NETNAME];
                    GetPlayerName(entNum, winnerName, sizeof(winnerName));
                    GetPlayerName(loserIdx, loserName, sizeof(loserName));

                    GVM_LogPrintf("DuelEnd: %s has defeated %s in a private duel\n", winnerName, loserName);
                }
            }

            // UNLOCK: Reset trackers so they can duel again
            oldDuelState[entNum] = qfalse;
            duelOpponent[entNum] = -1; // Use -1 to avoid accidental Player 0 triggers
        }
    }

    // --- 2. CULLING LOGIC (Smooth Visible Ghosting) ---
    if (!sv_snapShotDuelCull->integer)
        return 0;

    if (touch->s.eType != ET_PLAYER && touch->s.eType != ET_NPC) {
        return 0; 
    }

    playerState_t *ps = GetPS(ent);
    int touchNum = touch->s.number;

    // IF I AM DUELING: Ghost everyone except my opponent
    if (ps && ps->duelInProgress) {
        if (ps->duelIndex != touchNum) {
            return 2; // RETURN 2: Visible, but no hard collision
        }
        return 0; 
    }

    // IF I AM A BYSTANDER: Ghost the duelists
    playerState_t *ops = GetPS(touch);
    if (ops && ops->duelInProgress) {
        return 2; // Visible Ghost
    }

    return 0;
}    