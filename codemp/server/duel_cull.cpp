#include "qcommon/qcommon.h"
#include "server.h"         // This provides the actual definitions for the structs
#include "duel_cull.h"      // This provides your custom signature
#include "sv_gameapi.h"

// Persistent state trackers
static qboolean oldDuelState[MAX_CLIENTS] = { qfalse };
static int      duelOpponent[MAX_CLIENTS] = { 0 };
static char     cachedNames[MAX_CLIENTS][MAX_NETNAME];

static qboolean isPlayer(sharedEntity_t *ent) {
    return (ent->s.eType == ET_PLAYER) ? qtrue : qfalse;
}

// Helper to resolve missiles/events back to their owner
static sharedEntity_t *FlattenEntity(sharedEntity_t *ent) {
    if (!ent) return NULL;

    // Handle Thrown Sabers/Projectiles
    if (ent->s.eType == ET_MISSILE) {
        return SV_GentityNum(ent->r.ownerNum);
    }
    
    // Handle Saber Block/Hit Events (translates event to the player)
    if (ent->s.eType >= ET_EVENTS) {
        int owner = (ent->s.otherEntityNum2 == ENTITYNUM_NONE) ? ent->s.otherEntityNum : ent->s.otherEntityNum2;
        // Fallback to clientNum for certain player events
        if (owner < 0 || owner >= MAX_GENTITIES) {
            owner = ent->s.clientNum;
        }
        return SV_GentityNum(owner);
    }

    return ent;
}

// Helper to extract clean names for logging
static void GetPlayerName(int clientNum, char *outName, int maxSize) {
    if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
        Q_strncpyz(outName, "Unknown", maxSize);
        return;
    }

    // If the cache is empty, fill it once.
    if (!cachedNames[clientNum][0]) {
        char configstring[MAX_CONFIGSTRINGS];
        SV_GetConfigstring(CS_PLAYERS + clientNum, configstring, sizeof(configstring));
        const char *value = Info_ValueForKey(configstring, "n");
        
        char cleanName[MAX_NETNAME];
        Q_strncpyz(cleanName, (value && value[0]) ? value : "Player", sizeof(cleanName));
        Q_CleanStr(cleanName); 
        Q_strncpyz(cachedNames[clientNum], cleanName, MAX_NETNAME);
    }

    Q_strncpyz(outName, cachedNames[clientNum], maxSize);
}

// THE CLEAR CACHE FUNCTION (Hook this in sv_client.cpp)
void DuelCull_ClearCache(int clientNum) {
    if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
        cachedNames[clientNum][0] = '\0';
    }
}

int DuelCull(sharedEntity_t *ent, sharedEntity_t *touch, moveclip_t *clip) {
    if (!sv_snapShotDuelCull->integer) return 0;

    // Resolve 'touch' to its owner (e.g., saber missile -> player owner)
    sharedEntity_t *resolvedTouch = FlattenEntity(touch);
    if (!resolvedTouch) return 0;

    int entNum = ent->s.number;
    playerState_t *ps = (entNum >= 0 && entNum < MAX_CLIENTS) ? SV_GameClientNum(entNum) : NULL;

    // --- 1. LOGGING LOGIC ---
    if (ps && isPlayer(ent)) {
        qboolean isCurrentlyDueling = ps->duelInProgress ? qtrue : qfalse;

        if (isCurrentlyDueling && !oldDuelState[entNum]) {
            int myOpponentIdx = ps->duelIndex; 
            if (myOpponentIdx >= 0 && myOpponentIdx < MAX_CLIENTS && myOpponentIdx != entNum) {
                playerState_t *oppPs = SV_GameClientNum(myOpponentIdx);
                if (oppPs && oppPs->duelInProgress && oppPs->duelIndex == entNum) {
                    oldDuelState[entNum] = qtrue;
                    duelOpponent[entNum] = myOpponentIdx;
                    if (entNum < myOpponentIdx) {
                        char p1Name[MAX_NETNAME], p2Name[MAX_NETNAME];
                        GetPlayerName(entNum, p1Name, sizeof(p1Name));
                        GetPlayerName(myOpponentIdx, p2Name, sizeof(p2Name));
                        GVM_LogPrintf("DuelStart: %s challenged %s to a private duel\n", p1Name, p2Name);
                    }
                }
            }
        }
        else if (!isCurrentlyDueling && oldDuelState[entNum]) {
            if (ps->stats[STAT_HEALTH] > 1) {
                int loserIdx = duelOpponent[entNum];
                if (loserIdx >= 0 && loserIdx < MAX_CLIENTS) {
                    char winnerName[MAX_NETNAME], loserName[MAX_NETNAME];
                    GetPlayerName(entNum, winnerName, sizeof(winnerName));
                    GetPlayerName(loserIdx, loserName, sizeof(loserName));
                    GVM_LogPrintf("DuelEnd: %s has defeated %s in a private duel\n", winnerName, loserName);
                }
            }
            oldDuelState[entNum] = qfalse;
            duelOpponent[entNum] = -1; 
        }
    }

    // --- 2. CULLING LOGIC ---
    
    // Safety: Always keep map objects solid
    if (resolvedTouch->s.eType != ET_PLAYER && resolvedTouch->s.eType != ET_NPC) {
        return 0;
    }

    // NEW: FORCE STANDARD BOUNDING BOX FOR COMBAT
    // This ensures saber-clashes and hits feel 100% normal.
    if (clip && (clip->contentmask & (MASK_SHOT | CONTENTS_BODY))) {
        return 0; 
    }

    int touchOwnerNum = resolvedTouch->s.number;

    // DUELIST PERSPECTIVE
    if (ps && ps->duelInProgress) {
        // If the 'touch' (or its owner) is our opponent, keep it solid
        if (touchOwnerNum == ps->duelIndex) {
            return 0; 
        }
        
        // Hide NPCs/Dummies during duels
        if (resolvedTouch->s.eType == ET_NPC) return 1;

        // Ghost everyone else
        return 2;
    }

    // BYSTANDER PERSPECTIVE
    // Ghost players (or their sabers) that are currently in a duel
    if (resolvedTouch->s.eType == ET_PLAYER) {
        playerState_t *ops = SV_GameClientNum(touchOwnerNum);
        if (ops && ops->duelInProgress) {
            return 2; 
        }
    }

    return 0;
}