#include "kernel/types.h"
#include "user/user.h"

/* xv6 benchmark utility for creating scheduler workload patterns. */

enum workload_kind {
  WORKLOAD_CPU_BOUND = 0,
  WORKLOAD_MIXED = 1,
  WORKLOAD_WAKEUP_HEAVY = 2
};

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

/* Runs a CPU-heavy child workload. */
static void
run_cpu_bound(int rounds)
{
  busy_loop(rounds);
}

/* Runs a balanced workload with both compute and sleep. */
static void
run_mixed(int rounds)
{
  int i;

  for(i = 0; i < rounds; i++){
    busy_loop(3);
    sleep(5);
  }
}

/* Runs a bursty workload with frequent wakeups. */
static void
run_wakeup_heavy(int rounds)
{
  int i;

  for(i = 0; i < rounds; i++){
    busy_loop(1);
    sleep(1);
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

/* Assign a different workload size to each child.
   Child 1 gets the smallest job, child N gets the biggest. */
static int
child_rounds(int base_rounds, int child_index)
{
  return base_rounds * child_index;
}

/* Runs the requested workload in the child process. */
static void
run_workload(int workload, int rounds)
{
  if(workload == WORKLOAD_CPU_BOUND){
    run_cpu_bound(rounds);
  } else if(workload == WORKLOAD_MIXED){
    run_mixed(rounds);
  } else {
    run_wakeup_heavy(rounds);
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
  int workload;
  int children;
  int base_rounds;
  int i;

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

  half_point = (children + 1) / 2;

  start_tick = uptime();

  printf("greenbench: workload=%s children=%d base_rounds=%d\n",
         workload_name(workload), children, base_rounds);
  printf("greenbench: child workloads scale from %d x 1 up to %d x %d\n",
         base_rounds, base_rounds, children);
  printf("greenbench: start tick=%d\n", start_tick);

  for(i = 0; i < children; i++){
    int child_id = i + 1;
    int pid;
    int my_rounds = child_rounds(base_rounds, child_id);

    pid = fork();

    if(pid < 0){
      fprintf(2, "greenbench: fork failed\n");
      exit(1);
    }

    if(pid == 0){
      run_workload(workload, my_rounds);
      exit(0);
    } else {
      printf("greenbench: forked child %d with PID %d (rounds=%d)\n",
             child_id, pid, my_rounds);
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

  printf("greenbench: complete\n");
  exit(0);
}