#ifndef IDLE_STATE_H
#define IDLE_STATE_H

#include "types.h"

void green_idle_reset(void);
void green_idle_enter(uint now_tick);
void green_idle_exit(uint now_tick);
void green_idle_on_tick(void);
void green_idle_snapshot(uint *idle_ticks, uint *idle_entries, int *in_idle);

#endif
