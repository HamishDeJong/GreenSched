 #include "kernel/types.h"
 #include "kernel/stat.h"
 #include "kernel/param.h"
 #include "user/user.h"

 #define PIPESIZE 32

 struct bad_pipe {
 char data[PIPESIZE];
 uint nread; // number of bytes read
 uint nwrite; // number of bytes written
 };


 void
 pipe_write(struct bad_pipe *pi, char ch)
 {
    //writes one byte
   pi->data[pi->nwrite % PIPESIZE] = ch;
   pi->nwrite++;
 //TODO
 }

 int
 pipe_read(struct bad_pipe *pi,  char *ch)
 {
  if(pi->nread == pi->nwrite)
    return 0;   // empty

   *ch = pi->data[pi->nread % PIPESIZE];
   pi->nread++;
  return 1;
 //TODO
 }

 int
 main(void)
 {
 struct bad_pipe pipe;

 char ch;
 char last3[3] = {0, 0, 0}; // Track last 3 characters to detect "ok?"

 pipe.nread = 0;
 pipe.nwrite = 0;

 printf("Type text. Enter 'ok?' to stop and display buffer contents.\n\n");


 // Read from stdin
 while(read(0, &ch, 1) == 1){
 pipe_write(&pipe, ch);
 // Check for "ok?" pattern before writing
 last3[0] = last3[1];
 last3[1] = last3[2];
 last3[2] = ch;

 if(last3[0] == 'o' && last3[1] == 'k' && last3[2] == '?'){
    pipe.nwrite -= 3; // Remove the 'o' and 'k' and '?'
      break;
 //TODO
 }
}

    printf("\n--- Bad pipe output ---\n");

    while (pipe_read(&pipe, &ch)) {
    printf("%c", ch);
    }
    printf("\n");
    exit(0);

}