#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
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
    p->green_run_streak = 0;
}

// Called when process runs for one tick
void green_stats_on_run_tick(struct proc *p) {
    if (!p) return;
    p->cpu_ticks += 1;
    p->recent_cpu += 1;
    p->green_run_streak += 1;
}

// Called when process sleeps for one tick
void green_stats_on_sleep_tick(struct proc *p) {
    if (!p) return;
    p->sleep_ticks += 1;
    p->green_run_streak = 0;
    green_stats_decay_recent_cpu(p, 2);
}

// Called on wakeup
void green_stats_on_wakeup(struct proc *p) {
    if (!p) return;
    p->wakeups += 1;
    p->green_run_streak = 0;
    p->recent_cpu = p->recent_cpu / 2;
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

    uint wakeup_cost = (mode == ENERGY_MODE_ENERGY) ? 8 : 3;
    uint switch_cost = (mode == ENERGY_MODE_ENERGY) ? 6 : 2;
    uint sleep_credit = 0;
    uint score;

    score =
        clamp_window(p->cpu_ticks, 40) * 2 +
        p->wakeups * wakeup_cost +
        p->context_switches * switch_cost +
        clamp_window(p->recent_cpu, 40) * 6 +
        clamp_window(p->green_run_streak, 4) * 5;

    if(mode == ENERGY_MODE_ENERGY){
        sleep_credit = clamp_window(p->sleep_ticks, 40) * 5;
        if(sleep_credit >= score)
            score = 0;
        else
            score -= sleep_credit;
    }

    p->energy_score = score;
    return p->energy_score;
}
