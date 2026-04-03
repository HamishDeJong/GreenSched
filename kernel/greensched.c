#include "types.h"
#include "proc.h"   // for struct proc
#include "defs.h"   // for helper functions
#include "greensched.h"
/* Baseline scheduler selection logic for round-robin and green mode. */



/* Minimal process information needed for scheduler decisions. */
struct green_sched_candidate {
  int pid;
  int runnable;
  uint recent_cpu;
  uint wakeups;
  uint context_switches;
};



/* Returns the PID of the next process to run, or -1 if none are runnable. */
struct proc*
green_pick_next_process(int mode)
{
  struct proc *p;
  struct proc *best = 0;
  uint best_cost = 0xffffffff;

  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;

    if(mode == 0) // RR
      return p;

    uint cost = green_candidate_cost(p);

    if(cost < best_cost){
      best_cost = cost;
      best = p;
    }
  }

  return best;
}