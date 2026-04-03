// energy_accounting.h
#ifndef ENERGY_ACCOUNTING_H
#define ENERGY_ACCOUNTING_H

#include "types.h"  // uint

// Forward declaration of struct proc to avoid pulling in all of proc.h
struct proc;

// Scheduler modes
enum energy_mode {
    ENERGY_MODE_RR = 0,
    ENERGY_MODE_ENERGY = 1
};

// Per-process green scheduler/statistics hooks
void green_stats_reset(struct proc *p);
void green_stats_on_run_tick(struct proc *p);
void green_stats_on_sleep_tick(struct proc *p);
void green_stats_on_wakeup(struct proc *p);
void green_stats_on_context_switch(struct proc *p);
void green_stats_decay_recent_cpu(struct proc *p, uint decay_step);
uint green_compute_energy_score(struct proc *p, int mode);

#endif // ENERGY_ACCOUNTING_H