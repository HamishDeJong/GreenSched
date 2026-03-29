#include <stddef.h>

/* Baseline scheduler selection logic for round-robin and green mode. */

enum green_mode {
    GREEN_MODE_RR = 0,
    GREEN_MODE_ENERGY = 1
};

/* Minimal process information needed for scheduler decisions. */
struct green_sched_candidate {
    int pid;
    int runnable;
    unsigned int recent_cpu;
    unsigned int wakeups;
    unsigned int context_switches;
};

/* Lower cost means the process is a better fit for green scheduling. */
static unsigned int green_candidate_cost(const struct green_sched_candidate *candidate)
{
    /* Non-runnable entries should never be selected. */
    if (candidate == NULL || !candidate->runnable) {
        return 0xffffffffU;
    }

    return candidate->recent_cpu * 5U +
           candidate->wakeups * 3U +
           candidate->context_switches * 2U;
}

/* Returns the PID of the next process to run, or -1 if none are runnable. */
int green_pick_next_process(
    const struct green_sched_candidate *candidates,
    size_t count,
    int mode)
{
    size_t i;
    int best_pid;
    unsigned int best_cost;

    /* Reject empty candidate lists. */
    if (candidates == NULL || count == 0) {
        return -1;
    }

    /* Round-robin picks the first runnable process it sees. */
    if (mode == GREEN_MODE_RR) {
        /* Scan until a runnable entry is found. */
        for (i = 0; i < count; ++i) {
            /* Return the first runnable PID immediately. */
            if (candidates[i].runnable) {
                return candidates[i].pid;
            }
        }
        return -1;
    }

    best_pid = -1;
    best_cost = 0xffffffffU;

    /* Scan all candidates to find the lowest green cost. */
    for (i = 0; i < count; ++i) {
        unsigned int cost = green_candidate_cost(&candidates[i]);
        /* Keep the best runnable process seen so far. */
        if (cost < best_cost) {
            best_cost = cost;
            best_pid = candidates[i].pid;
        }
    }

    return best_pid;
}
