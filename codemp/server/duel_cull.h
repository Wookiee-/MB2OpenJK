#pragma once

#include "qcommon/qcommon.h"

// Exact match from sv_world.cpp to ensure memory alignment
typedef struct moveclip_s {
    vec3_t      boxmins, boxmaxs;
    const float *mins;
    const float *maxs;
    
    /* Ghoul2 Insert Start */
    vec3_t      start;
    vec3_t      end;

    int         passEntityNum;
    int         contentmask;
    int         capsule;

    int         traceFlags;
    int         useLod;
    trace_t     trace;
    /* Ghoul2 Insert End */
} moveclip_t;

struct sharedEntity_s;
typedef struct sharedEntity_s sharedEntity_t;

int DuelCull(sharedEntity_t *a, sharedEntity_t *b, moveclip_t *clip);