#include "kernel/types.h"
#include "user/user.h"

/* xv6 benchmark utility for creating scheduler workload patterns. */

/* Workload profiles used for benchmark generation. */
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

  /* Repeat work to keep the CPU busy. */
  for(i = 0; i < rounds; i++){
    /* Do a simple arithmetic loop each round. */
    for(j = 0; j < 5000; j++)
      sink += i + j;
  }
}

/* Runs a CPU-heavy child workload. */
static void
run_cpu_bound(int rounds)
{
  /* Spend all rounds doing compute work. */
  busy_loop(rounds);
}

/* Runs a balanced workload with both compute and sleep. */
static void
run_mixed(int rounds)
{
  int i;

  /* Alternate between work and sleeping. */
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

  /* Wake up often after short sleeps. */
  for(i = 0; i < rounds; i++){
    busy_loop(1);
    sleep(1);
  }
}

/* Parses a workload name. */
static int
parse_workload(char *value)
{
  /* Reject missing input. */
  if(value == 0)
    return -1;

  /* Parse the CPU-heavy workload. */
  if(strcmp(value, "cpu_bound") == 0)
    return WORKLOAD_CPU_BOUND;

  /* Parse the mixed CPU and sleep workload. */
  if(strcmp(value, "mixed") == 0)
    return WORKLOAD_MIXED;

  /* Parse the wakeup-heavy workload. */
  if(strcmp(value, "wakeup") == 0)
    return WORKLOAD_WAKEUP_HEAVY;

  return -1;
}

/* Runs the requested workload in the child process. */
static void
run_workload(int workload, int rounds)
{
  /* Run the CPU-heavy benchmark path. */
  if(workload == WORKLOAD_CPU_BOUND){
    run_cpu_bound(rounds);
  /* Run the mixed sleep and CPU benchmark path. */
  } else if(workload == WORKLOAD_MIXED){
    run_mixed(rounds);
  /* Run the frequent wakeup benchmark path. */
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
  int rounds;
  int i;

  /* Reject the wrong number of arguments. */
  if(argc != 4){
    print_usage(argv[0]);
    exit(1);
  }

  workload = parse_workload(argv[1]);

  /* Stop if the workload name is invalid. */
  if(workload < 0){
    print_usage(argv[0]);
    exit(1);
  }

  children = atoi(argv[2]);
  rounds = atoi(argv[3]);

  /* Stop if the child count is invalid. */
  if(children <= 0){
    print_usage(argv[0]);
    exit(1);
  }

  /* Stop if the round count is invalid. */
  if(rounds <= 0){
    print_usage(argv[0]);
    exit(1);
  }

  printf("greenbench: starting %d children\n", children);

  /* Fork each benchmark worker. */
  for(i = 0; i < children; i++){
    int pid;

    pid = fork();

    /* Stop if fork fails. */
    if(pid < 0){
      fprintf(2, "greenbench: fork failed\n");
      exit(1);
    }

    /* Run the workload in the child and then exit. */
    if(pid == 0){
      run_workload(workload, rounds);
      exit(0);
    }
  }

  /* Wait for all child processes to finish. */
  for(i = 0; i < children; i++)
    wait(0);

  printf("greenbench: complete\n");
  exit(0);
}
