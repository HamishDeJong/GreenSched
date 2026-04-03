// kernel/energy_idle.h
#ifndef IDLE_STATE_H
#define IDLE_STATE_H

//#include "types.h"

// Idle tracker functions
void green_idle_reset(void);
void green_idle_enter(uint now_tick);
void green_idle_exit(uint now_tick);
void green_idle_on_tick(void);

#endif // IDLE_STATE_H