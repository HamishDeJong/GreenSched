#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Generates simple synthetic metrics for scheduler comparisons. */

enum green_mode {
    GREEN_MODE_RR = 0,
    GREEN_MODE_ENERGY = 1
};

/* Workload profiles used for synthetic benchmark generation. */
enum workload_kind {
    WORKLOAD_CPU_BOUND = 0,
    WORKLOAD_MIXED = 1,
    WORKLOAD_WAKEUP_HEAVY = 2
};

/* One process row in the generated metrics file. */
struct benchmark_row {
    int pid;
    unsigned int cpu_ticks;
    unsigned int sleep_ticks;
    unsigned int wakeups;
    unsigned int context_switches;
    unsigned int energy_score;
};

/* Returns the path storing the selected scheduler mode. */
static const char *default_state_path(void)
{
    const char *env_path = getenv("GREENSCHED_MODE_FILE");
    /* Use the environment override when it is available. */
    return env_path != NULL && env_path[0] != '\0'
        ? env_path
        : "greensched_mode.state";
}

/* Returns the default output file for generated metrics. */
static const char *default_metrics_path(void)
{
    const char *env_path = getenv("GREENSCHED_METRICS_FILE");
    /* Use the environment override when it is available. */
    return env_path != NULL && env_path[0] != '\0'
        ? env_path
        : "greensched_metrics.csv";
}

/* Converts a mode value to a readable string. */
static const char *mode_name(int mode)
{
    return mode == GREEN_MODE_ENERGY ? "green" : "rr";
}

/* Converts a workload value to a readable string. */
static const char *workload_name(int workload)
{
    /* Return the label for the selected workload. */
    switch (workload) {
    case WORKLOAD_CPU_BOUND:
        return "cpu_bound";
    case WORKLOAD_MIXED:
        return "mixed";
    default:
        return "wakeup";
    }
}

/* Parses a scheduler mode string. */
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

/* Parses a workload name. */
static int parse_workload(const char *value, int *workload)
{
    /* Reject invalid arguments. */
    if (value == NULL || workload == NULL) {
        return -1;
    }

    /* Parse the CPU-heavy workload. */
    if (strcmp(value, "cpu_bound") == 0) {
        *workload = WORKLOAD_CPU_BOUND;
        return 0;
    }

    /* Parse the mixed CPU and sleep workload. */
    if (strcmp(value, "mixed") == 0) {
        *workload = WORKLOAD_MIXED;
        return 0;
    }

    /* Parse the wakeup-heavy workload. */
    if (strcmp(value, "wakeup") == 0) {
        *workload = WORKLOAD_WAKEUP_HEAVY;
        return 0;
    }

    return -1;
}

/* Reads the scheduler mode from the shared state file. */
static int read_mode_from_state(const char *path, int *mode)
{
    FILE *file;
    char buffer[32];

    /* Reject invalid output storage. */
    if (mode == NULL) {
        return -1;
    }

    file = fopen(path, "r");
    /* Default to round-robin if no saved mode exists yet. */
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

/* Computes a simple synthetic energy score for one process. */
static unsigned int compute_energy(
    int mode,
    unsigned int cpu_ticks,
    unsigned int wakeups,
    unsigned int context_switches)
{
    unsigned int cpu_cost = 5U;
    unsigned int wakeup_cost = mode == GREEN_MODE_ENERGY ? 4U : 5U;
    unsigned int switch_cost = mode == GREEN_MODE_ENERGY ? 2U : 4U;

    return cpu_ticks * cpu_cost +
           wakeups * wakeup_cost +
           context_switches * switch_cost;
}

/* Fills one synthetic process entry based on the selected workload. */
static void fill_row(struct benchmark_row *row, int mode, int workload, int index)
{
    unsigned int cpu_ticks;
    unsigned int sleep_ticks;
    unsigned int wakeups;
    unsigned int context_switches;

    row->pid = 100 + index;

    /* Use CPU-heavy values for compute-bound processes. */
    if (workload == WORKLOAD_CPU_BOUND) {
        cpu_ticks = 120U + (unsigned int)(index * 9);
        sleep_ticks = 6U + (unsigned int)(index % 3);
        wakeups = 2U + (unsigned int)(index % 2);
        context_switches = mode == GREEN_MODE_ENERGY ? 10U + (unsigned int)index : 18U + (unsigned int)(index * 2);
    /* Use balanced values for mixed workloads. */
    } else if (workload == WORKLOAD_MIXED) {
        cpu_ticks = 70U + (unsigned int)(index * 6);
        sleep_ticks = 40U + (unsigned int)(index * 5);
        wakeups = 6U + (unsigned int)(index % 4);
        context_switches = mode == GREEN_MODE_ENERGY ? 12U + (unsigned int)index : 20U + (unsigned int)(index * 2);
    /* Use high wakeup counts for bursty workloads. */
    } else {
        cpu_ticks = 48U + (unsigned int)(index * 4);
        sleep_ticks = 55U + (unsigned int)(index * 3);
        wakeups = 14U + (unsigned int)(index * 2);
        context_switches = mode == GREEN_MODE_ENERGY ? 16U + (unsigned int)index : 28U + (unsigned int)(index * 2);
    }

    row->cpu_ticks = cpu_ticks;
    row->sleep_ticks = sleep_ticks;
    row->wakeups = wakeups;
    row->context_switches = context_switches;
    row->energy_score = compute_energy(mode, cpu_ticks, wakeups, context_switches);
}

/* Writes benchmark metadata and per-process rows to a CSV-like file. */
static int write_metrics(
    const char *path,
    int mode,
    int workload,
    int process_count,
    unsigned int idle_ticks)
{
    FILE *file;
    int i;
    struct benchmark_row row;

    file = fopen(path, "w");
    /* Stop if the metrics file cannot be opened. */
    if (file == NULL) {
        return -1;
    }

    fprintf(file, "mode,%s\n", mode_name(mode));
    fprintf(file, "workload,%s\n", workload_name(workload));
    fprintf(file, "idle_ticks,%u\n", idle_ticks);
    fprintf(file, "pid,cpu_ticks,sleep_ticks,wakeups,context_switches,energy_score\n");

    /* Write one line per synthetic process. */
    for (i = 0; i < process_count; ++i) {
        fill_row(&row, mode, workload, i);
        fprintf(
            file,
            "%d,%u,%u,%u,%u,%u\n",
            row.pid,
            row.cpu_ticks,
            row.sleep_ticks,
            row.wakeups,
            row.context_switches,
            row.energy_score);
    }

    fclose(file);
    return 0;
}

/* Prints command-line usage. */
static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s [cpu_bound|mixed|wakeup] [count] [metrics-file]\n", program);
    fprintf(stderr, "   or: %s [rr|green] [cpu_bound|mixed|wakeup] [count] [metrics-file]\n", program);
}

/* Generates a benchmark file from the selected mode and workload. */
int main(int argc, char **argv)
{
    const char *metrics_path = default_metrics_path();
    int mode;
    int workload = WORKLOAD_CPU_BOUND;
    int process_count = 5;
    int argi = 1;
    unsigned int idle_ticks;

    /* Stop if the saved scheduler mode cannot be loaded. */
    if (read_mode_from_state(default_state_path(), &mode) != 0) {
        fprintf(stderr, "Unable to read scheduler mode state\n");
        return 1;
    }

    /* Allow the first argument to override the saved mode. */
    if (argc > argi && parse_mode(argv[argi], &mode) == 0) {
        argi += 1;
    }

    /* Allow the next argument to choose the workload type. */
    if (argc > argi && parse_workload(argv[argi], &workload) == 0) {
        argi += 1;
    }

    /* Allow the user to choose how many synthetic processes to generate. */
    if (argc > argi) {
        process_count = atoi(argv[argi]);
        /* Reject invalid process counts. */
        if (process_count <= 0) {
            print_usage(argv[0]);
            return 1;
        }
        argi += 1;
    }

    /* Allow the user to override the output file path. */
    if (argc > argi) {
        metrics_path = argv[argi];
        argi += 1;
    }

    /* Reject any extra arguments. */
    if (argc != argi) {
        print_usage(argv[0]);
        return 1;
    }

    idle_ticks = mode == GREEN_MODE_ENERGY ? 35U + (unsigned int)(process_count * 3) : 14U + (unsigned int)process_count;

    /* Stop if the generated metrics cannot be written. */
    if (write_metrics(metrics_path, mode, workload, process_count, idle_ticks) != 0) {
        fprintf(stderr, "Unable to write metrics file: %s\n", metrics_path);
        return 1;
    }

    printf("benchmark mode: %s\n", mode_name(mode));
    printf("workload: %s\n", workload_name(workload));
    printf("processes: %d\n", process_count);
    printf("metrics written to: %s\n", metrics_path);
    return 0;
}
