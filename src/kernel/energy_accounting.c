#include <stddef.h>

/* Basic helpers for tracking per-process sustainability metrics. */

enum green_mode {
    GREEN_MODE_RR = 0,
    GREEN_MODE_ENERGY = 1
};

/* Core metrics collected for one process. */
struct green_proc_stats {
    int pid;
    unsigned int cpu_ticks;
    unsigned int sleep_ticks;
    unsigned int wakeups;
    unsigned int context_switches;
    unsigned int recent_cpu;
    unsigned int energy_score;
};

/* Keeps recent CPU usage from growing without bound. */
static unsigned int clamp_window(unsigned int value, unsigned int window)
{
    return value > window ? window : value;
}

/* Initializes all counters for a process. */
void green_stats_reset(struct green_proc_stats *stats, int pid)
{
    /* Ignore invalid input. */
    if (stats == NULL) {
        return;
    }

    stats->pid = pid;
    stats->cpu_ticks = 0;
    stats->sleep_ticks = 0;
    stats->wakeups = 0;
    stats->context_switches = 0;
    stats->recent_cpu = 0;
    stats->energy_score = 0;
}

/* Updates counters when the process runs for one tick. */
void green_stats_on_run_tick(struct green_proc_stats *stats)
{
    /* Ignore invalid input. */
    if (stats == NULL) {
        return;
    }

    stats->cpu_ticks += 1;
    stats->recent_cpu += 1;
}

/* Records one tick of sleeping or blocked time. */
void green_stats_on_sleep_tick(struct green_proc_stats *stats)
{
    /* Ignore invalid input. */
    if (stats == NULL) {
        return;
    }

    stats->sleep_ticks += 1;
}

/* Records a wakeup event. */
void green_stats_on_wakeup(struct green_proc_stats *stats)
{
    /* Ignore invalid input. */
    if (stats == NULL) {
        return;
    }

    stats->wakeups += 1;
}

/* Records that the process was scheduled onto the CPU. */
void green_stats_on_context_switch(struct green_proc_stats *stats)
{
    /* Ignore invalid input. */
    if (stats == NULL) {
        return;
    }

    stats->context_switches += 1;
}

/* Gradually lowers recent CPU usage so the metric reflects recent activity. */
void green_stats_decay_recent_cpu(struct green_proc_stats *stats, unsigned int decay_step)
{
    /* Ignore invalid input. */
    if (stats == NULL) {
        return;
    }

    /* Clamp to zero if the decay is larger than the current value. */
    if (decay_step >= stats->recent_cpu) {
        stats->recent_cpu = 0;
        return;
    }

    stats->recent_cpu -= decay_step;
}

/* Computes a simple synthetic energy score from the collected counters. */
unsigned int green_compute_energy_score(struct green_proc_stats *stats, int mode)
{
    unsigned int wakeup_cost;
    unsigned int switch_cost;

    /* Return a neutral value for invalid input. */
    if (stats == NULL) {
        return 0;
    }

    /* Green mode slightly changes the synthetic cost weights. */
    wakeup_cost = mode == GREEN_MODE_ENERGY ? 4U : 3U;
    switch_cost = mode == GREEN_MODE_ENERGY ? 3U : 2U;

    stats->energy_score =
        stats->cpu_ticks * 5U +
        stats->wakeups * wakeup_cost +
        stats->context_switches * switch_cost +
        clamp_window(stats->recent_cpu, 100U);

    return stats->energy_score;
}
