#include "types.h"
#include "param.h"        // NCPU, NOFILE
#include "memlayout.h"    // pagetable_t, pte_t
#include "spinlock.h"     // struct spinlock
#include "proc.h"         // struct proc
#include "defs.h"         // helper functions
#include "energy_accounting.h"

// Helper to clamp recent CPU usage
static uint clamp_window(uint value, uint window) {
    return value > window ? window : value;
}

// Reset all counters for a process
void green_stats_reset(struct proc *p) {
    if (!p) return;
    p->cpu_ticks = 0;
    p->sleep_ticks = 0;
    p->wakeups = 0;
    p->context_switches = 0;
    p->recent_cpu = 0;
    p->energy_score = 0;
}

// Called when process runs for one tick
void green_stats_on_run_tick(struct proc *p) {
    if (!p) return;
    p->cpu_ticks += 1;
    p->recent_cpu += 1;
}

// Called when process sleeps for one tick
void green_stats_on_sleep_tick(struct proc *p) {
    if (!p) return;
    p->sleep_ticks += 1;
}

// Called on wakeup
void green_stats_on_wakeup(struct proc *p) {
    if (!p) return;
    p->wakeups += 1;
}

// Called on context switch
void green_stats_on_context_switch(struct proc *p) {
    if (!p) return;
    p->context_switches += 1;
}

// Decay recent CPU usage
void green_stats_decay_recent_cpu(struct proc *p, uint decay_step) {
    if (!p) return;
    if (decay_step >= p->recent_cpu) {
        p->recent_cpu = 0;
        return;
    }
    p->recent_cpu -= decay_step;
}

// Compute energy score
uint green_compute_energy_score(struct proc *p, int mode) {
    if (!p) return 0;

    uint wakeup_cost = (mode == ENERGY_MODE_ENERGY) ? 4 : 3;
    uint switch_cost = (mode == ENERGY_MODE_ENERGY) ? 3 : 2;

    p->energy_score =
        p->cpu_ticks * 5 +
        p->wakeups * wakeup_cost +
        p->context_switches * switch_cost +
        clamp_window(p->recent_cpu, 100);

    return p->energy_score;
}