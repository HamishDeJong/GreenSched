#include "kernel/types.h"
#include "user/user.h"



/* xv6 benchmark utility for creating scheduler workload patterns. */

enum workload_kind {
  WORKLOAD_CPU_BOUND = 0,
  WORKLOAD_MIXED = 1,
  WORKLOAD_WAKEUP_HEAVY = 2
};

enum wakeup_profile_kind {
  WAKEUP_PROFILE_CHATTY = 0,
  WAKEUP_PROFILE_CALM = 1,
  WAKEUP_PROFILE_HOG = 2
};

//int syscall(int num, ...);
//wrapper
// int
// setrecentcpu(int pid, int value)
// {
//     return syscall(26, pid, value);  
// }

/* Burns CPU time without sleeping. */
static void
busy_loop(int rounds)
{
  volatile int sink;
  int i;
  int j;

  sink = 0;

  for(i = 0; i < rounds; i++){
    for(j = 0; j < 5000; j++)
      sink += i + j;
  }
}

/* Burn CPU for a target number of scheduler ticks. */
static void
busy_ticks(int target_ticks)
{
  int start_tick;

  start_tick = uptime();
  while((uptime() - start_tick) < target_ticks)
    busy_loop(1);
}

/* Runs a CPU-heavy child workload. */
static void
run_cpu_bound(int rounds)
{
  busy_loop(rounds);
}

/* Runs a balanced workload with both compute and sleep. */
static void
run_mixed(int rounds, int child_id)
{
  int i;

  if(child_id % 2 == 1){
    /* Interactive profile: short bursts with light blocking. */
    for(i = 0; i < rounds; i++){
      busy_loop(1);
      if((i % 3) == 2)
        sleep(1);
    }
  } else {
    /* Batch profile: sustained CPU pressure measured in timer ticks. */
    for(i = 0; i < rounds; i++)
      busy_ticks(1);
  }
}

/* Runs a bursty workload with frequent wakeups. */
static void
run_wakeup_heavy(int rounds, int child_id)
{
  int i;
  int burst_ticks;
  int sleep_ticks;
  int cycles;

  if((child_id % 3) == 1){
    /* Chatty profile: many short wake/sleep cycles. */
    burst_ticks = 1;
    sleep_ticks = 1;
    cycles = rounds;
  } else if((child_id % 3) == 2){
    /* Calm profile: same overall shape, but fewer wakeups. */
    burst_ticks = 4;
    sleep_ticks = 4;
    cycles = rounds / 4;
    if(cycles < 1)
      cycles = 1;
  } else {
    /* Background hog: always runnable pressure competing with wakeups. */
    busy_ticks(rounds * 2);
    return;
  }

  for(i = 0; i < cycles; i++){
    busy_ticks(burst_ticks);
    sleep(sleep_ticks);
  }
}

/* Parses a workload name. */
static int
parse_workload(char *value)
{
  if(value == 0)
    return -1;

  if(strcmp(value, "cpu_bound") == 0)
    return WORKLOAD_CPU_BOUND;

  if(strcmp(value, "mixed") == 0)
    return WORKLOAD_MIXED;

  if(strcmp(value, "wakeup") == 0)
    return WORKLOAD_WAKEUP_HEAVY;

  return -1;
}

/* Converts workload enum to readable text. */
static char *
workload_name(int workload)
{
  if(workload == WORKLOAD_CPU_BOUND)
    return "cpu_bound";
  if(workload == WORKLOAD_MIXED)
    return "mixed";
  return "wakeup";
}

static char *
child_profile_name(int workload, int child_id)
{
  if(workload == WORKLOAD_WAKEUP_HEAVY){
    if((child_id % 3) == 1)
      return "chatty";
    if((child_id % 3) == 2)
      return "calm";

    return "hog";
  }

  if(workload != WORKLOAD_MIXED)
    return "standard";

  if(child_id % 2 == 1)
    return "interactive";

  return "batch";
}

/* Assign a different workload size to each child.
   Child 1 gets the smallest job, child N gets the biggest. */
static int
child_rounds(int base_rounds, int child_index)
{
  return base_rounds * child_index;
}

/* Keep wakeup-heavy children at the same size so scheduler behavior is visible. */
static int
benchmark_rounds(int workload, int base_rounds, int child_index)
{
  if(workload == WORKLOAD_WAKEUP_HEAVY)
    return base_rounds;

  if(workload == WORKLOAD_MIXED){
    if(child_index % 2 == 1)
      return base_rounds;
    return base_rounds * 2;
  }

  return child_rounds(base_rounds, child_index);
}

/* Add a small per-child phase offset so wakeups do not stay perfectly aligned. */
static void
apply_child_stagger(int workload, int child_id)
{
  int stagger_ticks;

  stagger_ticks = 0;

  if(workload == WORKLOAD_WAKEUP_HEAVY)
    stagger_ticks = (child_id - 1) % 4;
  else if(workload == WORKLOAD_MIXED){
    if(child_id % 2 == 1)
      stagger_ticks = 2 + ((child_id - 1) % 2);
    else
      stagger_ticks = 0;
  }

  if(stagger_ticks > 0)
    sleep(stagger_ticks);
}

/* Runs the requested workload in the child process. */
static void
run_workload(int workload, int rounds, int child_id)
{
  apply_child_stagger(workload, child_id);

  if(workload == WORKLOAD_CPU_BOUND){
    run_cpu_bound(rounds);
  } else if(workload == WORKLOAD_MIXED){
    run_mixed(rounds, child_id);
  } else {
    run_wakeup_heavy(rounds, child_id);
  }
}

/* Prints command-line usage. */
static void
print_usage(char *program)
{
  printf("usage: %s [cpu_bound|mixed|wakeup] [children] [rounds]\n", program);
}

/* Forks child processes to generate benchmark load inside xv6. */
int
main(int argc, char *argv[])
{
#define MAX_BENCH_CHILDREN 64
  int workload;
  int children;
  int base_rounds;
  int i;
  int child_pids[MAX_BENCH_CHILDREN];
  int child_profiles[MAX_BENCH_CHILDREN];

  int start_tick;
  int end_tick;
  int completion_sum;
  int best_elapsed;
  int worst_elapsed;

  int short_sum;
  int long_sum;
  int short_count;
  int long_count;

  int half_point;
  int interactive_sum;
  int batch_sum;
  int interactive_count;
  int batch_count;
  int chatty_sum;
  int calm_sum;
  int hog_sum;
  int chatty_count;
  int calm_count;
  int hog_count;

  if(argc != 4){
    print_usage(argv[0]);
    exit(1);
  }

  workload = parse_workload(argv[1]);

  if(workload < 0){
    print_usage(argv[0]);
    exit(1);
  }

  children = atoi(argv[2]);
  base_rounds = atoi(argv[3]);

  if(children <= 0){
    print_usage(argv[0]);
    exit(1);
  }

  if(children > MAX_BENCH_CHILDREN){
    fprintf(2, "greenbench: too many children (max %d)\n", MAX_BENCH_CHILDREN);
    exit(1);
  }

  if(base_rounds <= 0){
    print_usage(argv[0]);
    exit(1);
  }

  completion_sum = 0;
  best_elapsed = -1;
  worst_elapsed = 0;

  short_sum = 0;
  long_sum = 0;
  short_count = 0;
  long_count = 0;
  interactive_sum = 0;
  batch_sum = 0;
  interactive_count = 0;
  batch_count = 0;
  chatty_sum = 0;
  calm_sum = 0;
  hog_sum = 0;
  chatty_count = 0;
  calm_count = 0;
  hog_count = 0;

  half_point = (children + 1) / 2;

  start_tick = uptime();

  printf("greenbench: workload=%s children=%d base_rounds=%d\n",
         workload_name(workload), children, base_rounds);
  if(workload == WORKLOAD_WAKEUP_HEAVY){
    printf("greenbench: all child workloads use %d rounds for fair wakeup comparison\n",
           base_rounds);
    printf("greenbench: wakeup mode mixes chatty sleepers, calm sleepers, and background CPU hogs\n");
    printf("greenbench: every third child is a hog so wakeups compete with always-runnable work\n");
    printf("greenbench: child wakeups are lightly staggered to expose scheduler choices\n");
  } else if(workload == WORKLOAD_MIXED){
    printf("greenbench: interactive children use %d request-like rounds; batch children use %d CPU-heavy rounds\n",
           base_rounds, base_rounds * 2);
    printf("greenbench: batch jobs start immediately and interactive jobs arrive a few ticks later\n");
    printf("greenbench: odd children are interactive, even children are batch\n");
  } else {
    printf("greenbench: child workloads scale from %d x 1 up to %d x %d\n",
           base_rounds, base_rounds, children);
  }
  printf("greenbench: start tick=%d\n", start_tick);

  for(i = 0; i < children; i++){
    child_pids[i] = -1;
    child_profiles[i] = 0;
  }

  for(i = 0; i < children; i++){
    int child_id = i + 1;
    int pid;
    int my_rounds = benchmark_rounds(workload, base_rounds, child_id);

    pid = fork();

    if(pid < 0){
        fprintf(2, "greenbench: fork failed\n");
        exit(1);
    }

    if(pid == 0){
        // Child process: run workload
        run_workload(workload, my_rounds, child_id);
        exit(0);
    } else {
        // Parent process: print info
        printf("greenbench: forked child %d with PID %d (rounds=%d, profile=%s)\n",
               child_id, pid, my_rounds, child_profile_name(workload, child_id));
        child_pids[i] = pid;
        if(workload == WORKLOAD_MIXED){
          child_profiles[i] = (child_id % 2 == 1) ? 1 : 0;
        } else if(workload == WORKLOAD_WAKEUP_HEAVY){
          if((child_id % 3) == 1)
            child_profiles[i] = WAKEUP_PROFILE_CHATTY;
          else if((child_id % 3) == 2)
            child_profiles[i] = WAKEUP_PROFILE_CALM;
          else
            child_profiles[i] = WAKEUP_PROFILE_HOG;
        } else {
          child_profiles[i] = 0;
        }

        // Seed recent_cpu only for pure CPU scaling; mixed/wakeup stay scheduler-driven.
        if(workload == WORKLOAD_CPU_BOUND)
          setrecentcpu(pid, my_rounds);
    }
}

  for(i = 0; i < children; i++){
    int donepid = wait(0);
    int now = uptime();
    int elapsed = now - start_tick;

    printf("greenbench: completion #%d -> PID %d at tick %d (elapsed %d)\n",
           i + 1, donepid, now, elapsed);

    completion_sum += elapsed;

    if(best_elapsed < 0 || elapsed < best_elapsed)
      best_elapsed = elapsed;

    if(elapsed > worst_elapsed)
      worst_elapsed = elapsed;

    if(i < half_point){
      short_sum += elapsed;
      short_count++;
    } else {
      long_sum += elapsed;
      long_count++;
    }

    if(workload == WORKLOAD_MIXED){
      int child_index;

      for(child_index = 0; child_index < children; child_index++){
        if(child_pids[child_index] == donepid){
          if(child_profiles[child_index]){
            interactive_sum += elapsed;
            interactive_count++;
          } else {
            batch_sum += elapsed;
            batch_count++;
          }
          break;
        }
      }
    }

    if(workload == WORKLOAD_WAKEUP_HEAVY){
      int child_index;

      for(child_index = 0; child_index < children; child_index++){
        if(child_pids[child_index] == donepid){
          if(child_profiles[child_index] == WAKEUP_PROFILE_CHATTY){
            chatty_sum += elapsed;
            chatty_count++;
          } else if(child_profiles[child_index] == WAKEUP_PROFILE_CALM){
            calm_sum += elapsed;
            calm_count++;
          } else {
            hog_sum += elapsed;
            hog_count++;
          }
          break;
        }
      }
    }
  }

  end_tick = uptime();

  printf("greenbench: total elapsed = %d ticks\n", end_tick - start_tick);
  printf("greenbench: average completion = %d ticks\n",
         completion_sum / children);
  printf("greenbench: best = %d ticks, worst = %d ticks\n",
         best_elapsed, worst_elapsed);

  if(short_count > 0)
    printf("greenbench: early completions average = %d ticks\n",
           short_sum / short_count);

  if(long_count > 0)
    printf("greenbench: late completions average = %d ticks\n",
           long_sum / long_count);

  if(workload == WORKLOAD_MIXED && interactive_count > 0)
    printf("greenbench: interactive average = %d ticks\n",
           interactive_sum / interactive_count);

  if(workload == WORKLOAD_MIXED && batch_count > 0)
    printf("greenbench: batch average = %d ticks\n",
           batch_sum / batch_count);

  if(workload == WORKLOAD_WAKEUP_HEAVY && chatty_count > 0)
    printf("greenbench: chatty average = %d ticks\n",
           chatty_sum / chatty_count);

  if(workload == WORKLOAD_WAKEUP_HEAVY && calm_count > 0)
    printf("greenbench: calm average = %d ticks\n",
           calm_sum / calm_count);

  if(workload == WORKLOAD_WAKEUP_HEAVY && hog_count > 0)
    printf("greenbench: hog average = %d ticks\n",
           hog_sum / hog_count);

  printf("greenbench: complete\n");
  exit(0);
}
