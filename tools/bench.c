/* bench: compare answers, then measure each approach independently.
 *
 * Wall min/median cover spawn through wait; CPU median is child user plus
 * system time. RSS is peak child resident memory in KiB. Linux instr is
 * median user-space instructions over five runs; work is the median and
 * range of paired differences subtracting process startup (an empty-input
 * run). Interleaving limits drift bias; unavailable counters print n/a.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#include "aoc.h"
#include "util.h"

static int cmpdbl(const void *a, const void *b);
static char *output(char *const cmd[], const char *input, int which,
                    int *failed);
static void run(char *bin, const char *input, double *wall, double *cpu);
static void measure(char *bin, const char *input, size_t runs);
#ifdef __linux__
static int cmplong(const void *a, const void *b);
static int instructions(char *bin, const char *input, long *total);
static void counters(char *bin, const char *input);
#endif
static int benchmark(char *bin, const char *input, size_t runs,
                     const char *oracle, const char *want);

static int
cmpdbl(const void *a, const void *b)
{
	const double *x = a, *y = b;

	return (*x > *y) - (*x < *y);
}

static char *
output(char *const cmd[], const char *input, int which, int *failed)
{
	char *out;
	int fd;

	fd = xopen(input, O_RDONLY);
	out = capture(cmd, fd, which, failed);
	if (close(fd) < 0)
		die("bench: close:");
	return out;
}

static void
run(char *bin, const char *input, double *wall, double *cpu)
{
	char *cmd[] = {bin, NULL};
	struct rusage before, after;
	struct timespec t0, t1;
	int fd, null;

	fd = xopen(input, O_RDONLY);
	null = xopen("/dev/null", O_WRONLY);
	if (getrusage(RUSAGE_CHILDREN, &before) < 0 ||
	    clock_gettime(CLOCK_MONOTONIC, &t0) < 0)
		die("bench: clock:");
	if (reap(spawn(cmd, fd, null, -1)))
		die("bench: %s failed", bin);
	if (clock_gettime(CLOCK_MONOTONIC, &t1) < 0 ||
	    getrusage(RUSAGE_CHILDREN, &after) < 0)
		die("bench: clock:");
	if (close(fd) < 0 || close(null) < 0)
		die("bench: close:");
	*wall = ((double)(t1.tv_sec - t0.tv_sec) * 1e6) +
	        ((double)(t1.tv_nsec - t0.tv_nsec) / 1e3);
	*cpu = ((double)(after.ru_utime.tv_sec - before.ru_utime.tv_sec +
	                 after.ru_stime.tv_sec - before.ru_stime.tv_sec) *
	        1e6) +
	       (double)(after.ru_utime.tv_usec - before.ru_utime.tv_usec +
	                after.ru_stime.tv_usec - before.ru_stime.tv_usec);
}

static void
measure(char *bin, const char *input, size_t runs)
{
	struct rusage ru;
	double *wall, *cpu;
	const char *name;
	size_t i;
	long rss;

	wall = ecalloc(runs, 2 * sizeof(*wall));
	cpu = wall + runs;
	for (i = 0; i < runs; i++)
		run(bin, input, &wall[i], &cpu[i]);
	qsort(wall, runs, sizeof(*wall), cmpdbl);
	qsort(cpu, runs, sizeof(*cpu), cmpdbl);
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		die("bench: getrusage:");
	rss = ru.ru_maxrss;
#ifdef __APPLE__
	rss /= 1024;
#endif
	name = strrchr(bin, '/');
	name = name ? name + 1 : bin;
	if (printf("%-16s wall min %8.1f med %8.1f us  cpu med %8.1f us  "
	           "rss %6ld KiB",
	           name, wall[0], wall[runs / 2], cpu[runs / 2], rss) < 0)
		die("bench: printf:");
	free(wall);
}

#ifdef __linux__
static int
cmplong(const void *a, const void *b)
{
	const long *x = a, *y = b;

	return (*x > *y) - (*x < *y);
}

static int
instructions(char *bin, const char *input, long *total)
{
	char prog[] = "perf", stat[] = "stat", sep[] = "-x,", event[] = "-e",
	     user[] = "instructions:u", dash[] = "--";
	char *cmd[] = {prog, stat, sep, event, user, dash, bin, NULL};
	char *out, *line, *end;
	long n;
	int failed, found = 0;

	out = output(cmd, input, 2, &failed);
	*total = 0;
	for (line = strtok(out, "\n"); line && !failed;
	     line = strtok(NULL, "\n")) {
		errno = 0;
		n = strtol(line, &end, 10);
		if (end == line || *end != ',' || n < 0 ||
		    !strstr(end, "instructions"))
			continue;
		if (errno || n > LONG_MAX - *total) {
			failed = 1;
			break;
		}
		*total += n;
		found = 1;
	}
	free(out);
	return !failed && found;
}

static void
counters(char *bin, const char *input)
{
	long full[5], work[5], empty;
	size_t i;

	for (i = 0; i < LEN(full); i++) {
		if (!instructions(bin, input, &full[i]) ||
		    !instructions(bin, "/dev/null", &empty))
			break;
		work[i] = full[i] - empty;
	}
	if (i == LEN(full)) {
		qsort(full, LEN(full), sizeof(*full), cmplong);
		qsort(work, LEN(work), sizeof(*work), cmplong);
		if (printf("  instr %ld work %ld (%ld-%ld, n=5)", full[2],
		           work[2], work[0], work[4]) < 0)
			die("bench: printf:");
	} else if (printf("  instr n/a") < 0) {
		die("bench: printf:");
	}
}
#endif

static int
benchmark(char *bin, const char *input, size_t runs, const char *oracle,
          const char *want)
{
	char *cmd[] = {bin, NULL}, *out;
	pid_t pid;
	int failed;

	out = output(cmd, input, 1, &failed);
	if (failed) {
		if (printf("FAIL %s: failed\n", bin) < 0)
			die("bench: printf:");
	} else if (strcmp(out, want) != 0) {
		failed = 1;
		if (printf("FAIL %s: answers differ from %s\n", bin, oracle) <
		    0)
			die("bench: printf:");
	}
	free(out);
	if (failed)
		return 1;
	if (fflush(stdout) == EOF)
		die("bench: fflush:");
	/* RUSAGE_CHILDREN's peak RSS is a maximum over every child ever
	 * waited for. A fresh process isolates each binary's maximum. */
	if ((pid = fork()) < 0)
		die("bench: fork:");
	if (pid == 0) {
		measure(bin, input, runs);
		if (fflush(stdout) == EOF)
			die("bench: fflush:");
		_exit(0);
	}
	if (reap(pid))
		return 1;
#ifdef __linux__
	counters(bin, input);
#endif
	if (putchar('\n') == EOF)
		die("bench: putchar:");
	return 0;
}

int
main(int argc, char *argv[])
{
	char *cmd[] = {NULL, NULL}, *end, *want;
	long runs;
	int i, failed;

	if (argc < 4)
		die("bench: usage: bench RUNS INPUT BIN...");
	errno = 0;
	runs = strtol(argv[1], &end, 10);
	if (errno || end == argv[1] || *end || runs < 1 || runs > 100000)
		die("bench: runs must be 1-100000: %s", argv[1]);
	cmd[0] = argv[3];
	want = output(cmd, argv[2], 1, &failed);
	if (failed && printf("FAIL %s: failed\n", argv[3]) < 0)
		die("bench: printf:");
	for (i = 3; i < argc && !failed; i++)
		failed =
		    benchmark(argv[i], argv[2], (size_t)runs, argv[3], want);
	free(want);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
