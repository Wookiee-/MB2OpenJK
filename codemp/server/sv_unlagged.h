#ifndef SV_UNLAGGED_H
#define SV_UNLAGGED_H

#include "server.h"

// 1. Define the structure FIRST
typedef struct {
    int     time;
    vec3_t  origin;
    vec3_t  mins, maxs;
    vec3_t  angles;
    vec3_t saberBase; 
    vec3_t saberTip;  
} sv_history_t;

// 2. Define the constants
#define MAX_UNLAG_HISTORY 100 

// 3. The global bridge pointer
extern client_t *sv_unlagged_client;

// 4. Function prototypes
void SV_Unlagged_StoreHistory(void);
void SV_Unlagged_RewindAll(int ping);
void SV_Unlagged_RestoreAll(void);

#endif