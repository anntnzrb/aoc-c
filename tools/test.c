/* test: run every built solution against every fixture, all years
 *
 * usage: test
 *
 * For each tests/YEAR/DD/NAME.in, every binary in bin/YEAR/DD/ must exit 0
 * and print the first k lines of NAME.out, where k is the number of lines
 * in NAME.out: a one-line .out tests part 1 alone while part 2 is open.
 */
#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aoc.h"
#include "util.h"

static void bars(char *s);
static int compare(char *bin, const char *in, const char *want, size_t k);
static int fixture(const char *in, const glob_t *bins);
static int directory(const char *dir);

static void
bars(char *s)
{
	for (; *s; s++)
		if (*s == '\n')
			*s = '|';
}

static int
compare(char *bin, const char *in, const char *want, size_t k)
{
	char *cmd[] = {bin, NULL};
	char *got, *p, *display;
	int fd, failed;
	size_t n = 0;

	fd = xopen(in, O_RDONLY);
	got = capture(cmd, fd, 1, &failed);
	if (close(fd) < 0)
		die("test: close:");
	for (p = got; *p && n < k; p++)
		if (*p == '\n')
			n++;
	*p = '\0';
	if (failed) {
		if (printf("FAIL %s %s: failed\n", bin, in) < 0)
			die("test: printf:");
	} else if (strcmp(got, want) != 0) {
		failed = 1;
		display = smprintf("%s", want);
		bars(display);
		bars(got);
		if (printf("FAIL %s %s: want '%s' got '%s'\n", bin, in,
		           display, got) < 0)
			die("test: printf:");
		free(display);
	} else if (printf("ok   %s %s\n", bin, in) < 0) {
		die("test: printf:");
	}
	free(got);
	return failed;
}

static int
fixture(const char *in, const glob_t *bins)
{
	FILE *fp;
	char *out, *want;
	size_t len, i, k = 0;
	int failed = 0;

	out = smprintf("%.*s.out", (int)(strlen(in) - 3), in);
	if (!(fp = fopen(out, "r"))) {
		if (printf("FAIL %s: open failed\n", out) < 0)
			die("test: printf:");
		free(out);
		return 1;
	}
	want = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("test: fclose:");
	for (i = 0; i < len; i++)
		if (want[i] == '\n')
			k++;
	if (k == 0 || want[len - 1] != '\n') {
		if (printf("FAIL %s: empty or no trailing newline\n", out) < 0)
			die("test: printf:");
		failed = 1;
	} else {
		for (i = 0; i < bins->gl_pathc; i++)
			failed |= compare(bins->gl_pathv[i], in, want, k);
	}
	free(want);
	free(out);
	return failed;
}

static int
directory(const char *dir)
{
	glob_t bins, fixtures;
	char *pat;
	size_t i, n;
	int failed = 0;

	pat = smprintf("bin/%s/*", dir + strlen("tests/"));
	if (!xglob(pat, &bins)) {
		if (printf("FAIL %s: no builds\n", dir) < 0)
			die("test: printf:");
		failed = 1;
	}
	free(pat);
	pat = smprintf("%s/*.in", dir);
	n = bins.gl_pathc ? xglob(pat, &fixtures) : 0;
	for (i = 0; i < n; i++)
		failed |= fixture(fixtures.gl_pathv[i], &bins);
	if (n)
		globfree(&fixtures);
	globfree(&bins);
	free(pat);
	return failed;
}

int
main(int argc, char *argv[])
{
	glob_t dirs;
	size_t i, n;
	int failed = 0;

	(void)argv;
	if (argc != 1)
		die("usage: test");
	n = xglob("tests/[0-9][0-9][0-9][0-9]/[0-9][0-9]/", &dirs);
	for (i = 0; i < n; i++) {
		dirs.gl_pathv[i][strlen(dirs.gl_pathv[i]) - 1] = '\0';
		failed |= directory(dirs.gl_pathv[i]);
	}
	globfree(&dirs);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
