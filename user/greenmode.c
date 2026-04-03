#include "kernel/types.h"
#include "user/user.h"

/* xv6 utility for getting or changing the current scheduler mode. */

enum green_mode {
  GREEN_MODE_RR = 0,
  GREEN_MODE_ENERGY = 1
};

/*
 * Expected xv6 syscalls:
 *   int getgreenmode(void);
 *   int setgreenmode(int mode);
 */
int getgreenmode(void);
int setgreenmode(int mode);

/* Converts a mode value to a readable label. */
static char*
mode_name(int mode)
{
  return mode == GREEN_MODE_ENERGY ? "green" : "rr";
}

/* Parses text such as "rr" or "green" into a mode constant. */
static int
parse_mode(char *value, int *mode)
{
  /* Reject invalid arguments. */
  if(value == 0 || mode == 0)
    return -1;

  /* Parse round-robin mode. */
  if(strcmp(value, "rr") == 0){
    *mode = GREEN_MODE_RR;
    return 0;
  }

  /* Parse energy-aware mode. */
  if(strcmp(value, "green") == 0){
    *mode = GREEN_MODE_ENERGY;
    return 0;
  }

  return -1;
}

/* Prints command-line usage. */
static void
print_usage(void)
{
  printf("usage: greenmode [get|rr|green]\n");
}

/* Supports get, rr, and green commands. */
int
main(int argc, char *argv[])
{
  char *command;
  int mode;

  command = "get";

  /* Use the first argument as the command when provided. */
  if(argc >= 2)
    command = argv[1];

  /* Reject extra arguments. */
  if(argc > 2){
    print_usage();
    exit(1);
  }

  /* Print the current mode without changing it. */
  if(strcmp(command, "get") == 0){
    mode = getgreenmode();

    /* Stop if the kernel does not return a valid mode. */
    if(mode < 0){
      fprintf(2, "greenmode: getgreenmode failed\n");
      exit(1);
    }

    printf("scheduler mode: %s\n", mode_name(mode));
    exit(0);
  }

  /* Parse an explicit mode name. */
  if(parse_mode(command, &mode) != 0){
    print_usage();
    exit(1);
  }

  /* Stop if the kernel rejects the requested mode. */
  if(setgreenmode(mode) < 0){
    fprintf(2, "greenmode: setgreenmode failed\n");
    exit(1);
  }

  printf("scheduler mode set to: %s\n", mode_name(mode));
  exit(0);
}
