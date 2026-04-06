// kernel/green.h
#ifndef GREENSCHED_H
#define GREENSCHED_H



// Function declarations
struct proc* green_pick_next_process(int mode);
uint green_candidate_cost(struct proc *p, int mode);
int green_should_preempt(struct proc *p, int mode);

// You can also declare the other green_* functions if needed
void green_stats_reset(struct proc *p);
void green_stats_on_run_tick(struct proc *p);
void green_stats_on_context_switch(struct proc *p);
void green_stats_on_wakeup(struct proc *p);
void green_stats_on_sleep_tick(struct proc *p);
void green_stats_decay_recent_cpu(struct proc *p, uint decay_step);
uint green_compute_energy_score(struct proc *p, int mode);

#endif
