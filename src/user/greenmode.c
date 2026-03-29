#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Small utility for getting or changing the current scheduler mode. */

enum green_mode {
    GREEN_MODE_RR = 0,
    GREEN_MODE_ENERGY = 1
};

/* Returns the scheduler state file path. */
static const char *default_state_path(void)
{
    const char *env_path = getenv("GREENSCHED_MODE_FILE");
    /* Use the environment override when it is available. */
    return env_path != NULL && env_path[0] != '\0'
        ? env_path
        : "greensched_mode.state";
}

/* Converts a mode value to a readable label. */
static const char *mode_name(int mode)
{
    return mode == GREEN_MODE_ENERGY ? "green" : "rr";
}

/* Parses text such as "rr" or "green" into a mode constant. */
static int parse_mode(const char *value, int *mode)
{
    /* Reject invalid arguments. */
    if (value == NULL || mode == NULL) {
        return -1;
    }

    /* Parse round-robin mode. */
    if (strcmp(value, "rr") == 0) {
        *mode = GREEN_MODE_RR;
        return 0;
    }

    /* Parse energy-aware mode. */
    if (strcmp(value, "green") == 0) {
        *mode = GREEN_MODE_ENERGY;
        return 0;
    }

    return -1;
}

/* Reads the current mode from the state file. */
static int read_mode(const char *path, int *mode)
{
    FILE *file;
    char buffer[32];

    /* Reject invalid output storage. */
    if (mode == NULL) {
        return -1;
    }

    file = fopen(path, "r");
    /* Default to round-robin if no saved state exists yet. */
    if (file == NULL) {
        *mode = GREEN_MODE_RR;
        return 0;
    }

    /* Default to round-robin if the file cannot be read. */
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        *mode = GREEN_MODE_RR;
        return 0;
    }

    fclose(file);
    return parse_mode(strtok(buffer, "\r\n\t "), mode);
}

/* Writes the selected mode to the state file. */
static int write_mode(const char *path, int mode)
{
    FILE *file = fopen(path, "w");
    /* Report failure if the state file cannot be opened. */
    if (file == NULL) {
        return -1;
    }

    fprintf(file, "%s\n", mode_name(mode));
    fclose(file);
    return 0;
}

/* Prints command-line usage. */
static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s [get|rr|green|toggle] [state-file]\n", program);
}

/* Supports get, rr, green, and toggle commands. */
int main(int argc, char **argv)
{
    const char *state_path = default_state_path();
    const char *command = "get";
    int mode;

    /* Use the first argument as the command when provided. */
    if (argc >= 2) {
        command = argv[1];
    }

    /* Use a custom state file when provided. */
    if (argc >= 3) {
        state_path = argv[2];
    }

    /* Stop if the current state cannot be loaded. */
    if (read_mode(state_path, &mode) != 0) {
        fprintf(stderr, "Unable to read scheduler mode from %s\n", state_path);
        return 1;
    }

    /* Print the current mode without changing it. */
    if (strcmp(command, "get") == 0) {
        printf("scheduler mode: %s\n", mode_name(mode));
        return 0;
    }

    /* Flip between the two supported scheduler modes. */
    if (strcmp(command, "toggle") == 0) {
        mode = (mode == GREEN_MODE_RR) ? GREEN_MODE_ENERGY : GREEN_MODE_RR;
    /* Parse an explicit mode name. */
    } else if (parse_mode(command, &mode) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    /* Stop if the updated mode cannot be saved. */
    if (write_mode(state_path, mode) != 0) {
        fprintf(stderr, "Unable to write scheduler mode to %s\n", state_path);
        return 1;
    }

    printf("scheduler mode set to: %s\n", mode_name(mode));
    return 0;
}
