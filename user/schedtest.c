#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"






int burst_ticks;
// Dummy calculation function to simulate CPU burst
void cpu_burst(int iterations) {
    int start = uptime();        // ticks since boot
    while (uptime() - start < iterations*10) {
            // busy wait: burn CPU
    }
}


// void io_burst() {
//     pause(0); // simulate I/O once per iteration
// }

void child_process(int child_id, int increasing) {
    int burst_input, j;

    if (increasing) {
        burst_ticks = child_id * 10;
    } else {
        burst_ticks = (6 - child_id) * 10;
    }

    for (j = 0; j < 3; j++) {
        burst_input = 1 + getpid();
        cpu_burst(burst_input);
       // io_burst();

    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: schedtest [-inc | -dec]\n");
        exit(1);
    }

    int increasing = 1;

    if (strcmp(argv[1], "-dec") == 0) {
        increasing = 0;
    } else if (strcmp(argv[1], "-inc") == 0) {
        increasing = 1;
    } else {
        printf("Usage: schedtest [-inc | -dec]\n");
        exit(1);
    }

    int i;
    
    for (i = 0; i < 5; i++) {
        int pid = fork();
        
        if (pid < 0) {
            printf("Fork failed for child %d\n", i);
            exit(1);
        } else if (pid == 0) {
            
            child_process(i + 1, increasing);
            exit(0);  
        } else {
            
            printf("Parent: Forked child %d with PID %d\n", i + 1, pid);
        }
    }
    
    
   
    // waits for all chidren to finish
    for (i = 0; i < 5; i++) {
        wait(0);
    }


    
    
    exit(0);
}
