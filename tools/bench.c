/* bench: compare answers, then measure each approach independently.
 *
 * Wall min/median cover spawn through wait; CPU median is child user plus
 * system time. RSS is peak child resident memory in KiB. Linux instr is
 * median user-space instructions over five runs; work is the median and
 * range of paired differences subtracting process startup (an empty-input
 * run). Interleaving limits drift bias; unavailable counters print n/a.
 * Optional -o FILE -s STD -c CC before RUNS appends the 19-column shared
 * benchmark CSV schema, with invocation metadata and quoted text fields.
 * Timing/RSS come back from the isolated measuring child through a pipe.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include "aoc.h"
#include "util.h"

typedef struct {
	double wall_min, wall_med, cpu_med;
	long rss, work_med, work_min, work_max, instr_med;
	bool perf;
} Result;

typedef struct {
	FILE *file;
	char *text[7];
} Record;

static int
cmpdbl(const void *a, const void *b)
{
	const double *x = a, *y = b;

	return (*x > *y) - (*x < *y);
}

static char *
output(char *const cmd[], const char *input, int which, bool *failed)
{
	int fd = xopen(input, O_RDONLY);
	char *out = capture(cmd, fd, which, failed);
	if (close(fd) < 0)
		die("bench: close:");
	return out;
}

static char *
firstline(char *const cmd[])
{
	bool failed;
	char *out = capture(cmd, -1, 1, &failed);
	if (failed)
		die("bench: %s failed", cmd[0]);
	out[strcspn(out, "\r\n")] = '\0';
	return out;
}

static char *
cpuname(void)
{
#ifdef __linux__
	FILE *file = fopen("/proc/cpuinfo", "r");
	if (!file)
		die("bench: /proc/cpuinfo:");
	char *line = NULL, *cpu = NULL;
	size_t cap = 0;
	while (getline(&line, &cap, file) >= 0) {
		char *value;
		if (strncmp(line, "model name", 10) != 0 ||
		    !(value = strchr(line, ':')))
			continue;
		value++;
		value += strspn(value, " \t");
		value[strcspn(value, "\r\n")] = '\0';
		cpu = smprintf("%s", value);
		break;
	}
	if (ferror(file) || fclose(file) == EOF)
		die("bench: cpuinfo:");
	free(line);
	return cpu ? cpu : smprintf("unknown");
#elifdef __APPLE__
	char prog[] = "sysctl", flag[] = "-n",
	     key[] = "machdep.cpu.brand_string";
	char *cmd[] = {prog, flag, key, NULL};

	return firstline(cmd);
#else
	return smprintf("unknown");
#endif
}

static char *
quoted(const char *s)
{
	size_t n;
	if (ckd_add(&n, strlen(s), 2))
		die("bench: quoted string too large");
	char *out = ecalloc(n, 2), *p = out;
	*p++ = '"';
	for (; *s; s++) {
		if (*s == '"')
			*p++ = '"';
		*p++ = *s;
	}
	*p++ = '"';
	*p = '\0';
	return out;
}

static void
recording(Record *record, char *file, const char *std, char *cc)
{
	char git[] = "git", rev[] = "rev-parse", shortflag[] = "--short",
	     head[] = "HEAD", status[] = "status", porcelain[] = "--porcelain",
	     tracked[] = "--untracked-files=no", version[] = "--version",
	     dashes[] = "--";
	/* the CSV being appended is not a source change */
	char *except = smprintf(":(exclude)%s", file);
	char *rev_cmd[] = {git, rev, shortflag, head, NULL};
	char *status_cmd[] = {git,    status, porcelain, tracked,
	                      dashes, except, NULL};
	char *cc_cmd[] = {cc, version, NULL};
	struct stat st;

	if (!(record->file = fopen(file, "a")))
		die("bench: %s:", file);
	if (fstat(fileno(record->file), &st) < 0)
		die("bench: stat:");
	if (!st.st_size &&
	    fputs(
	        "date,commit,host,os,cpu,compiler,std,year,day,solution,runs,"
	        "work_med,work_min,work_max,instr_med,wall_min_us,wall_med_us,"
	        "cpu_med_us,rss_kib\n",
	        record->file) == EOF)
		die("bench: header:");
	char date[21];
	struct utsname host;
	const struct tm *utc;
	time_t now = time(NULL);
	if (now == (time_t)-1 || !(utc = gmtime(&now)) ||
	    !strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc) ||
	    uname(&host) < 0)
		die("bench: metadata:");
	char *commit = firstline(rev_cmd), *dirty = firstline(status_cmd);
	char *id = smprintf("%s%s", commit, *dirty ? "+dirty" : "");
	char *os =
	    smprintf("%s %s %s", host.sysname, host.release, host.machine);
	char *cpu = cpuname(), *compiler = firstline(cc_cmd);
	const char *fields[] = {date,     id, host.nodename, os, cpu,
	                        compiler, std};
	for (size_t i = 0; i < LEN(fields); i++)
		record->text[i] = quoted(fields[i]);
	free(commit);
	free(dirty);
	free(except);
	free(id);
	free(os);
	free(cpu);
	free(compiler);
}

static void
recordrow(const Record *record, char *bin, size_t runs, const Result *result)
{
	char *path = smprintf("%s", bin);
	char *solution = strrchr(path, '/');
	if (!solution)
		die("bench: invalid binary path: %s", bin);
	*solution++ = '\0';
	char *day = strrchr(path, '/');
	if (!day)
		die("bench: invalid binary path: %s", bin);
	*day++ = '\0';
	const char *year = strrchr(path, '/');
	if (!year)
		die("bench: invalid binary path: %s", bin);
	year++;
	int y, d;
	(void)puzzleday(year, day, &y, &d);
	char *name = quoted(solution), *row = NULL;
	size_t len = 0;
	FILE *stream = open_memstream(&row, &len);
	if (!stream)
		die("bench: open_memstream:");
	for (size_t i = 0; i < LEN(record->text); i++)
		if (fprintf(stream, "%s,", record->text[i]) < 0)
			die("bench: row:");
	if (fprintf(stream, "%d,%d,%s,%zu,", y, d, name, runs) < 0)
		die("bench: row:");
	if (result->perf) {
		if (fprintf(stream, "%ld,%ld,%ld,%ld,", result->work_med,
		            result->work_min, result->work_max,
		            result->instr_med) < 0)
			die("bench: row:");
	} else if (fputs(",,,,", stream) == EOF) {
		die("bench: row:");
	}
	if (fprintf(stream, "%.1f,%.1f,%.1f,%ld\n", result->wall_min,
	            result->wall_med, result->cpu_med, result->rss) < 0 ||
	    fclose(stream) == EOF)
		die("bench: row:");
	if (fwrite(row, 1, len, record->file) != len ||
	    fflush(record->file) == EOF)
		die("bench: write record:");
	free(row);
	free(name);
	free(path);
}

static void
run(char *bin, const char *input, double *wall, double *cpu)
{
	char *cmd[] = {bin, NULL};
	struct rusage before, after;
	struct timespec t0, t1;
	int fd = xopen(input, O_RDONLY), null = xopen("/dev/null", O_WRONLY);
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
measure(char *bin, const char *input, size_t runs, Result *result)
{
	double *wall = ecalloc(runs, 2 * sizeof(*wall)), *cpu = wall + runs;
	for (size_t i = 0; i < runs; i++)
		run(bin, input, &wall[i], &cpu[i]);
	qsort(wall, runs, sizeof(*wall), cmpdbl);
	qsort(cpu, runs, sizeof(*cpu), cmpdbl);
	struct rusage ru;
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		die("bench: getrusage:");
	long rss = ru.ru_maxrss;
#ifdef __APPLE__
	rss /= 1024;
#endif
	const char *name = strrchr(bin, '/');
	name = name ? name + 1 : bin;
	if (printf("%-16s wall min %8.1f med %8.1f us  cpu med %8.1f us  "
	           "rss %6ld KiB",
	           name, wall[0], wall[runs / 2], cpu[runs / 2], rss) < 0)
		die("bench: printf:");
	result->wall_min = wall[0];
	result->wall_med = wall[runs / 2];
	result->cpu_med = cpu[runs / 2];
	result->rss = rss;
	free(wall);
}

static bool
measured(char *bin, const char *input, size_t runs, Result *result)
{
	if (fflush(stdout) == EOF)
		die("bench: fflush:");
	int fd[2];
	xpipe(fd);
	/* A fresh process isolates RUSAGE_CHILDREN's lifetime peak RSS. */
	pid_t pid = fork();
	if (pid < 0)
		die("bench: fork:");
	if (pid == 0) {
		FILE *stream;
		if (close(fd[0]) < 0 || !(stream = fdopen(fd[1], "w")))
			die("bench: pipe:");
		measure(bin, input, runs, result);
		if (fwrite(result, sizeof(*result), 1, stream) != 1 ||
		    fclose(stream) == EOF || fflush(stdout) == EOF)
			die("bench: write measurement:");
		_exit(0);
	}
	FILE *stream;
	if (close(fd[1]) < 0 || !(stream = fdopen(fd[0], "r")))
		die("bench: pipe:");
	size_t n = fread(result, sizeof(*result), 1, stream);
	if (ferror(stream) || fclose(stream) == EOF)
		die("bench: read measurement:");
	return reap(pid) || n != 1;
}

#ifdef __linux__
static int
cmplong(const void *a, const void *b)
{
	const long *x = a, *y = b;

	return (*x > *y) - (*x < *y);
}

static bool
instructions(char *bin, const char *input, long *total)
{
	char prog[] = "perf", stat[] = "stat", sep[] = "-x,", event[] = "-e",
	     user[] = "instructions:u", dash[] = "--";
	char *cmd[] = {prog, stat, sep, event, user, dash, bin, NULL};
	bool failed, found = false;
	char *out = output(cmd, input, 2, &failed);
	*total = 0;
	for (char *line = strtok(out, "\n"); line && !failed;
	     line = strtok(NULL, "\n")) {
		errno = 0;
		char *end;
		long n = strtol(line, &end, 10);
		if (end == line || *end != ',' || n < 0 ||
		    !strstr(end, "instructions"))
			continue;
		if (errno || ckd_add(total, *total, n)) {
			failed = true;
			break;
		}
		found = true;
	}
	free(out);
	return !failed && found;
}

static void
counters(char *bin, const char *input, Result *result)
{
	long full[5], work[5], empty;
	size_t i = 0;
	for (; i < LEN(full); i++) {
		if (!instructions(bin, input, &full[i]) ||
		    !instructions(bin, "/dev/null", &empty))
			break;
		work[i] = full[i] - empty;
	}
	if (i == LEN(full)) {
		qsort(full, LEN(full), sizeof(*full), cmplong);
		qsort(work, LEN(work), sizeof(*work), cmplong);
		result->perf = true;
		result->instr_med = full[2];
		result->work_med = work[2];
		result->work_min = work[0];
		result->work_max = work[4];
		if (printf("  instr %ld work %ld (%ld-%ld, n=5)", full[2],
		           work[2], work[0], work[4]) < 0)
			die("bench: printf:");
	} else if (printf("  instr n/a") < 0) {
		die("bench: printf:");
	}
}
#endif

static bool
benchmark(char *bin, const char *input, size_t runs, const char *oracle,
          const char *want, const Record *record)
{
	char *cmd[] = {bin, NULL};
	bool failed;
	char *out = output(cmd, input, 1, &failed);
	if (failed) {
		if (printf("FAIL %s: failed\n", bin) < 0)
			die("bench: printf:");
	} else if (strcmp(out, want) != 0) {
		failed = true;
		if (printf("FAIL %s: answers differ from %s\n", bin, oracle) <
		    0)
			die("bench: printf:");
	}
	free(out);
	if (failed)
		return true;
	Result result = {};
	if (measured(bin, input, runs, &result))
		return true;
#ifdef __linux__
	counters(bin, input, &result);
#endif
	if (putchar('\n') == EOF)
		die("bench: putchar:");
	if (record->file)
		recordrow(record, bin, runs, &result);
	return false;
}

int
main(int argc, char *argv[])
{
	Record record = {};
	if (argc > 1 && strcmp(argv[1], "-o") == 0) {
		if (argc < 10 || strcmp(argv[3], "-s") != 0 ||
		    strcmp(argv[5], "-c") != 0)
			die("bench: usage: bench [-o FILE -s STD -c CC] "
			    "RUNS INPUT BIN...");
		recording(&record, argv[2], argv[4], argv[6]);
		argc -= 6;
		argv += 6;
	}
	if (argc < 4)
		die("bench: usage: bench [-o FILE -s STD -c CC] RUNS INPUT "
		    "BIN...");
	errno = 0;
	char *end;
	long runs = strtol(argv[1], &end, 10);
	if (errno || end == argv[1] || *end || runs < 1 || runs > 100000)
		die("bench: runs must be 1-100000: %s", argv[1]);
	char *cmd[] = {argv[3], NULL};
	bool failed;
	char *want = output(cmd, argv[2], 1, &failed);
	if (failed && printf("FAIL %s: failed\n", argv[3]) < 0)
		die("bench: printf:");
	for (int i = 3; i < argc && !failed; i++)
		failed = benchmark(argv[i], argv[2], (size_t)runs, argv[3],
		                   want, &record);
	free(want);
	if (record.file && fclose(record.file) == EOF)
		die("bench: close record:");
	for (size_t j = 0; j < LEN(record.text); j++)
		free(record.text[j]);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
