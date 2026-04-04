#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
cpu_burst(int ticks)
{
  int start = uptime();
  while((uptime() - start) < ticks){
    // busy wait
  }
}

int
compute_burst(int child_id, int increasing)
{
  if(increasing){
    return child_id * 10;        // 10,20,30,40,50
  } else {
    return (6 - child_id) * 10;  // 50,40,30,20,10
  }
}

void
child_process(int child_id, int increasing)
{
  int burst_ticks = compute_burst(child_id, increasing);
  int j;

  for(j = 0; j < 3; j++){
    cpu_burst(burst_ticks);
  }

  exit(0);
}

int
main(int argc, char *argv[])
{
  int increasing;
  int i;

  if(argc != 2){
    printf("Usage: schedtest [-inc | -dec]\n");
    exit(1);
  }

  if(strcmp(argv[1], "-inc") == 0){
    increasing = 1;
  } else if(strcmp(argv[1], "-dec") == 0){
    increasing = 0;
  } else {
    printf("Usage: schedtest [-inc | -dec]\n");
    exit(1);
  }

  printf("schedtest: mode=%s\n", increasing ? "increasing" : "decreasing");

  for(i = 0; i < 5; i++){
    int child_id = i + 1;
    int burst = compute_burst(child_id, increasing);
    int pid = fork();

    if(pid < 0){
      printf("schedtest: fork failed\n");
      exit(1);
    }

    if(pid == 0){
      child_process(child_id, increasing);
    } else {
      printf("Parent: Forked child %d with PID %d (burst=%d)\n",
             child_id, pid, burst);
    }
  }

  for(i = 0; i < 5; i++){
    int donepid = wait(0);
    printf("Parent: Child PID %d completed\n", donepid);
  }

  printf("schedtest complete\n");
  exit(0);
}