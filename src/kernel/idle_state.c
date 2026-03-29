#include <stddef.h>

/* Tracks simulated idle time for the system. */

/* Stores idle timing state and counters. */
struct green_idle_tracker {
    unsigned int idle_ticks;
    unsigned int idle_entries;
    unsigned int last_idle_start;
    int in_idle;
};

/* Clears all idle counters. */
void green_idle_reset(struct green_idle_tracker *tracker)
{
    /* Ignore invalid input. */
    if (tracker == NULL) {
        return;
    }

    tracker->idle_ticks = 0;
    tracker->idle_entries = 0;
    tracker->last_idle_start = 0;
    tracker->in_idle = 0;
}

/* Marks the beginning of an idle period. */
void green_idle_enter(struct green_idle_tracker *tracker, unsigned int now_tick)
{
    /* Ignore invalid input or repeated idle entry. */
    if (tracker == NULL || tracker->in_idle) {
        return;
    }

    tracker->idle_entries += 1;
    tracker->last_idle_start = now_tick;
    tracker->in_idle = 1;
}

/* Ends an idle period and adds its duration to the total. */
void green_idle_exit(struct green_idle_tracker *tracker, unsigned int now_tick)
{
    /* Ignore invalid input or exit attempts when not idle. */
    if (tracker == NULL || !tracker->in_idle) {
        return;
    }

    /* Only add positive elapsed idle time. */
    if (now_tick > tracker->last_idle_start) {
        tracker->idle_ticks += (now_tick - tracker->last_idle_start);
    }

    tracker->in_idle = 0;
}

/* Optional per-tick idle accounting helper. */
void green_idle_on_tick(struct green_idle_tracker *tracker)
{
    /* Only count ticks while the system is marked idle. */
    if (tracker == NULL || !tracker->in_idle) {
        return;
    }

    tracker->idle_ticks += 1;
}
