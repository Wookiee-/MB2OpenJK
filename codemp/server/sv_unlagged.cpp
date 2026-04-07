#include "server.h"
#include "sv_unlagged.h"

// The Global Bridge
client_t *sv_unlagged_client = NULL;

// History storage
static sv_history_t s_history[MAX_CLIENTS][MAX_UNLAG_HISTORY];
static int          s_historyHead[MAX_CLIENTS];
static vec3_t       s_backupOrigin[MAX_CLIENTS];
static vec3_t       s_backupAngles[MAX_CLIENTS];
static vec3_t       s_backupSaberBase[MAX_CLIENTS];
static vec3_t       s_backupSaberTip[MAX_CLIENTS];
static bool         s_isRewound = false;

/*
==================
SV_Unlagged_StoreHistory
==================
*/
void SV_Unlagged_StoreHistory(void) {
    for (int i = 0; i < sv_maxclients->integer; i++) {
        client_t *cl = &svs.clients[i];
        if (cl->state != CS_ACTIVE || !cl->gentity) continue;

        int head = s_historyHead[i];
        sv_history_t *h = &s_history[i][head];

        h->time = svs.time;
        VectorCopy(cl->gentity->r.currentOrigin, h->origin);
        VectorCopy(cl->gentity->r.mins, h->mins);
        VectorCopy(cl->gentity->r.maxs, h->maxs);
        VectorCopy(cl->gentity->r.currentAngles, h->angles);

        // Capture saber positions if your headers support them
        VectorCopy(cl->gentity->s.origin2, h->saberBase);
        VectorCopy(cl->gentity->s.angles2, h->saberTip);

        s_historyHead[i] = (head + 1) % MAX_UNLAG_HISTORY;
    }
}

/*
==================
SV_Unlagged_RewindAll
==================
*/
void SV_Unlagged_RewindAll(int ping) {
    if (s_isRewound || !sv_unlagged_client) return;

    int frameTime = (sv_fps->value > 0) ? (1000 / sv_fps->value) : 25;
    // FIXED: Changed (frameTime * 4) to (frameTime * 1) to remove the 100ms lag penalty
    int targetTime = svs.time - (ping + (frameTime * 1));

    for (int i = 0; i < sv_maxclients->integer; i++) {
        client_t *cl = &svs.clients[i];
        if (cl->state != CS_ACTIVE || !cl->gentity || cl == sv_unlagged_client) continue;

        VectorCopy(cl->gentity->r.currentOrigin, s_backupOrigin[i]);
        VectorCopy(cl->gentity->r.currentAngles, s_backupAngles[i]);

        sv_history_t *frameA = NULL;
        sv_history_t *frameB = NULL;

        for (int j = 0; j < MAX_UNLAG_HISTORY; j++) {
            sv_history_t *h = &s_history[i][j];
            if (h->time <= 0 || h->time > svs.time) continue;
            if (h->time <= targetTime) { if (!frameA || h->time > frameA->time) frameA = h; }
            if (h->time >= targetTime) { if (!frameB || h->time < frameB->time) frameB = h; }
        }

        if (frameA && frameB && frameA != frameB) {
            float frac = (float)(targetTime - frameA->time) / (float)(frameB->time - frameA->time);
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;

            for (int k = 0; k < 3; k++) {
                float lerpPos = frameA->origin[k] + frac * (frameB->origin[k] - frameA->origin[k]);
                float lerpAng = frameA->angles[k] + frac * (frameB->angles[k] - frameA->angles[k]);

                cl->gentity->r.currentOrigin[k] = lerpPos;
                cl->gentity->s.pos.trBase[k]    = lerpPos; 
                cl->gentity->s.origin[k]        = lerpPos;
                cl->gentity->r.currentAngles[k] = lerpAng;
                cl->gentity->s.apos.trBase[k]   = lerpAng;
                cl->gentity->s.angles[k]        = lerpAng;

                // MBII FIX: Rewind the saber base/tip so blocks work
                cl->gentity->s.origin2[k] = frameA->saberBase[k] + frac * (frameB->saberBase[k] - frameA->saberBase[k]);
                cl->gentity->s.angles2[k] = frameA->saberTip[k] + frac * (frameB->saberTip[k] - frameA->saberTip[k]);
            }
            VectorCopy(frameA->mins, cl->gentity->r.mins);
            VectorCopy(frameA->maxs, cl->gentity->r.maxs);
            SV_LinkEntity(cl->gentity);
        } else if (frameA) {
            VectorCopy(frameA->origin, cl->gentity->r.currentOrigin);
            VectorCopy(frameA->origin, cl->gentity->s.pos.trBase);
            VectorCopy(frameA->origin, cl->gentity->s.origin);
            VectorCopy(frameA->angles, cl->gentity->r.currentAngles);
            // MBII FIX: Snap sabers if LERP fails
            VectorCopy(frameA->saberBase, cl->gentity->s.origin2);
            VectorCopy(frameA->saberTip, cl->gentity->s.angles2);
            VectorCopy(frameA->mins, cl->gentity->r.mins);
            VectorCopy(frameA->maxs, cl->gentity->r.maxs);
            SV_LinkEntity(cl->gentity);
        }
    }
    s_isRewound = true;
}

/*
==================
SV_Unlagged_RestoreAll
==================
*/
void SV_Unlagged_RestoreAll(void) {
    if (!s_isRewound) return;

    for (int i = 0; i < sv_maxclients->integer; i++) {
        client_t *cl = &svs.clients[i];
        
        if (cl->state != CS_ACTIVE || !cl->gentity) continue;

        // Restore Body
        VectorCopy(s_backupOrigin[i], cl->gentity->r.currentOrigin);
        VectorCopy(s_backupOrigin[i], cl->gentity->s.pos.trBase);
        VectorCopy(s_backupOrigin[i], cl->gentity->s.origin);
        
        // Restore Angles
        VectorCopy(s_backupAngles[i], cl->gentity->r.currentAngles);
        VectorCopy(s_backupAngles[i], cl->gentity->s.apos.trBase);
        VectorCopy(s_backupAngles[i], cl->gentity->s.angles);

        // Restore MBII Sabers
        VectorCopy(s_backupSaberBase[i], cl->gentity->s.origin2);
        VectorCopy(s_backupSaberTip[i], cl->gentity->s.angles2);
        
        SV_LinkEntity(cl->gentity); 
    }

    s_isRewound = false;
    sv_unlagged_client = NULL; 
}