#pragma once

#include "server.h"

int DuelCull(sharedEntity_t *a, sharedEntity_t *b, playerState_t *ps);
void DuelCull_ClearCache(int clientNum);