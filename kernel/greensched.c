#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "greensched.h"

extern struct proc proc[];
/* Baseline scheduler selection logic for round-robin and green mode. */



/* Minimal process information needed for scheduler decisions. */
struct green_sched_candidate {
  int pid;
  int runnable;
  uint recent_cpu;
  uint wakeups;
  uint context_switches;
};




struct proc*
green_pick_next_process(int mode)
{
    static int last_rr_index = -1;
    static int last_green_index = -1;

    struct proc *p;
    struct proc *best = 0;
    uint best_cost = 0xffffffff;
    int i;

    if(mode == 0){  // RR
        int start = (last_rr_index + 1) % NPROC;

        for(i = 0; i < NPROC; i++){
            int idx = (start + i) % NPROC;
            if(proc[idx].state == RUNNABLE){
                last_rr_index = idx;
                return &proc[idx];
            }
        }
        return 0;
    }

    // Green mode with fairness
    int start = (last_green_index + 1) % NPROC;

    for(i = 0; i < NPROC; i++){
        int idx = (start + i) % NPROC;
        p = &proc[idx];

        if(p->state != RUNNABLE)
            continue;

        uint cost = green_candidate_cost(p);

        if(best == 0 || cost < best_cost){
            best_cost = cost;
            best = p;
            last_green_index = idx;
        }
    }

    return best;
}