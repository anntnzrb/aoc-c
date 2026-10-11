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
#include <limits.h>
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
	int perf;
} Result;

typedef struct {
	FILE *file;
	char *text[7];
} Record;

static int cmpdbl(const void *a, const void *b);
static char *output(char *const cmd[], const char *input, int which,
                    int *failed);
static char *firstline(char *const cmd[]);
static char *cpuname(void);
static char *quoted(const char *s);
static void recording(Record *record, char *file, const char *std, char *cc);
static void recordrow(const Record *record, char *bin, size_t runs,
                      const Result *result);
static void run(char *bin, const char *input, double *wall, double *cpu);
static void measure(char *bin, const char *input, size_t runs, Result *result);
static int measured(char *bin, const char *input, size_t runs, Result *result);
#ifdef __linux__
static int cmplong(const void *a, const void *b);
static int instructions(char *bin, const char *input, long *total);
static void counters(char *bin, const char *input, Result *result);
#endif
static int benchmark(char *bin, const char *input, size_t runs,
                     const char *oracle, const char *want,
                     const Record *record);

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

static char *
firstline(char *const cmd[])
{
	char *out;
	int failed;

	out = capture(cmd, -1, 1, &failed);
	if (failed)
		die("bench: %s failed", cmd[0]);
	out[strcspn(out, "\r\n")] = '\0';
	return out;
}

static char *
cpuname(void)
{
#ifdef __linux__
	FILE *file;
	char *line = NULL, *value, *cpu = NULL;
	size_t cap = 0;

	if (!(file = fopen("/proc/cpuinfo", "r")))
		die("bench: /proc/cpuinfo:");
	while (getline(&line, &cap, file) >= 0) {
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
#elif defined(__APPLE__)
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
	char *out, *p;
	size_t n;

	n = strlen(s);
	out = ecalloc(n + 2, 2);
	p = out;
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
	     tracked[] = "--untracked-files=no", version[] = "--version";
	char *rev_cmd[] = {git, rev, shortflag, head, NULL};
	char *status_cmd[] = {git, status, porcelain, tracked, NULL};
	char *cc_cmd[] = {cc, version, NULL};
	char date[21], *commit, *dirty, *os, *cpu, *compiler, *id;
	const char *fields[7];
	struct utsname host;
	struct stat st;
	const struct tm *utc;
	time_t now;
	size_t i;

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
	if ((now = time(NULL)) == (time_t)-1 || !(utc = gmtime(&now)) ||
	    !strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", utc) ||
	    uname(&host) < 0)
		die("bench: metadata:");
	commit = firstline(rev_cmd);
	dirty = firstline(status_cmd);
	id = smprintf("%s%s", commit, *dirty ? "+dirty" : "");
	os = smprintf("%s %s %s", host.sysname, host.release, host.machine);
	cpu = cpuname();
	compiler = firstline(cc_cmd);
	fields[0] = date;
	fields[1] = id;
	fields[2] = host.nodename;
	fields[3] = os;
	fields[4] = cpu;
	fields[5] = compiler;
	fields[6] = std;
	for (i = 0; i < LEN(fields); i++)
		record->text[i] = quoted(fields[i]);
	free(commit);
	free(dirty);
	free(id);
	free(os);
	free(cpu);
	free(compiler);
}

static void
recordrow(const Record *record, char *bin, size_t runs, const Result *result)
{
	char *path, *solution, *day, *name, *row = NULL;
	const char *year;
	FILE *stream;
	size_t len = 0, i;
	int y, d;

	path = smprintf("%s", bin);
	if (!(solution = strrchr(path, '/')))
		die("bench: invalid binary path: %s", bin);
	*solution++ = '\0';
	if (!(day = strrchr(path, '/')))
		die("bench: invalid binary path: %s", bin);
	*day++ = '\0';
	if (!(year = strrchr(path, '/')))
		die("bench: invalid binary path: %s", bin);
	year++;
	(void)puzzleday(year, day, &y, &d);
	name = quoted(solution);
	if (!(stream = open_memstream(&row, &len)))
		die("bench: open_memstream:");
	for (i = 0; i < LEN(record->text); i++)
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
measure(char *bin, const char *input, size_t runs, Result *result)
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
	result->wall_min = wall[0];
	result->wall_med = wall[runs / 2];
	result->cpu_med = cpu[runs / 2];
	result->rss = rss;
	free(wall);
}

static int
measured(char *bin, const char *input, size_t runs, Result *result)
{
	FILE *stream;
	pid_t pid;
	int fd[2];
	size_t n;

	if (fflush(stdout) == EOF)
		die("bench: fflush:");
	xpipe(fd);
	/* A fresh process isolates RUSAGE_CHILDREN's lifetime peak RSS. */
	if ((pid = fork()) < 0)
		die("bench: fork:");
	if (pid == 0) {
		if (close(fd[0]) < 0 || !(stream = fdopen(fd[1], "w")))
			die("bench: pipe:");
		measure(bin, input, runs, result);
		if (fwrite(result, sizeof(*result), 1, stream) != 1 ||
		    fclose(stream) == EOF || fflush(stdout) == EOF)
			die("bench: write measurement:");
		_exit(0);
	}
	if (close(fd[1]) < 0 || !(stream = fdopen(fd[0], "r")))
		die("bench: pipe:");
	n = fread(result, sizeof(*result), 1, stream);
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
counters(char *bin, const char *input, Result *result)
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
		result->perf = 1;
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

static int
benchmark(char *bin, const char *input, size_t runs, const char *oracle,
          const char *want, const Record *record)
{
	char *cmd[] = {bin, NULL}, *out;
	Result result = {0};
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
	if (measured(bin, input, runs, &result))
		return 1;
#ifdef __linux__
	counters(bin, input, &result);
#endif
	if (putchar('\n') == EOF)
		die("bench: putchar:");
	if (record->file)
		recordrow(record, bin, runs, &result);
	return 0;
}

int
main(int argc, char *argv[])
{
	char *cmd[] = {NULL, NULL}, *end, *want;
	Record record = {0};
	long runs;
	size_t j;
	int i, failed;

	if (argc > 1 && !strcmp(argv[1], "-o")) {
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
	runs = strtol(argv[1], &end, 10);
	if (errno || end == argv[1] || *end || runs < 1 || runs > 100000)
		die("bench: runs must be 1-100000: %s", argv[1]);
	cmd[0] = argv[3];
	want = output(cmd, argv[2], 1, &failed);
	if (failed && printf("FAIL %s: failed\n", argv[3]) < 0)
		die("bench: printf:");
	for (i = 3; i < argc && !failed; i++)
		failed = benchmark(argv[i], argv[2], (size_t)runs, argv[3],
		                   want, &record);
	free(want);
	if (record.file && fclose(record.file) == EOF)
		die("bench: close record:");
	for (j = 0; j < LEN(record.text); j++)
		free(record.text[j]);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
