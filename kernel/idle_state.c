#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "idle_state.h"

/* Tracks simulated idle time for the system. */

struct green_idle_tracker {
  uint idle_ticks;
  uint idle_entries;
  uint last_idle_start;
  int in_idle;
};

struct green_idle_tracker green_idle;

/* Reset all idle counters */
void
green_idle_reset(void)
{
  green_idle.idle_ticks = 0;
  green_idle.idle_entries = 0;
  green_idle.last_idle_start = 0;
  green_idle.in_idle = 0;
}

/* Enter idle */
void
green_idle_enter(uint now_tick)
{
  if(green_idle.in_idle)
    return;

  green_idle.idle_entries += 1;
  green_idle.last_idle_start = now_tick;
  green_idle.in_idle = 1;
}

/* Exit idle */
void
green_idle_exit(uint now_tick)
{
  if(!green_idle.in_idle)
    return;

  if(now_tick > green_idle.last_idle_start)
    green_idle.idle_ticks += (now_tick - green_idle.last_idle_start);

  green_idle.in_idle = 0;
}

/* Optional tick-based tracking */
void
green_idle_on_tick(void)
{
  if(!green_idle.in_idle)
    return;

  green_idle.idle_ticks += 1;
}

void
green_idle_snapshot(uint *idle_ticks, uint *idle_entries, int *in_idle)
{
  if(idle_ticks)
    *idle_ticks = green_idle.idle_ticks;
  if(idle_entries)
    *idle_entries = green_idle.idle_entries;
  if(in_idle)
    *in_idle = green_idle.in_idle;
}
