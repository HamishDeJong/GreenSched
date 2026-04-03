#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int fd[2];
  pipe(fd);

  int pid = fork();

  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }

  if(pid == 0){ // checks if child
    
    close(fd[1]);   // close write end

    char ch;
    while(read(fd[0], &ch, 1) == 1){
      printf("%c", ch);
    }

    close(fd[0]);
    exit(0);
  } 
  else { // else is parent
    
    close(fd[0]);   // close read end

    char *haiku =
      "White blossoms open.\n"
      "Hungry bees are caressed,\n"
      "in soft, love chambers.\n";

    write(fd[1], haiku, strlen(haiku));

    close(fd[1]);   // signals EOF to child
    wait(0);        // wait for child to finish
  }

  exit(0);
}
