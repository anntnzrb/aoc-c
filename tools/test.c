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

static void
bars(char *s)
{
	for (; *s; s++)
		if (*s == '\n')
			*s = '|';
}

static bool
compare(char *bin, const char *in, const char *want, size_t k)
{
	char *cmd[] = {bin, NULL};
	int fd = xopen(in, O_RDONLY);
	bool failed;
	char *got = capture(cmd, fd, 1, &failed);
	if (close(fd) < 0)
		die("test: close:");
	size_t n = 0;
	char *p = got;
	for (; *p && n < k; p++)
		if (*p == '\n')
			n++;
	*p = '\0';
	if (failed) {
		if (printf("FAIL %s %s: failed\n", bin, in) < 0)
			die("test: printf:");
	} else if (strcmp(got, want) != 0) {
		failed = true;
		char *display = smprintf("%s", want);
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

static bool
fixture(const char *in, const glob_t *bins)
{
	char *out = smprintf("%.*s.out", (int)(strlen(in) - 3), in);
	FILE *fp = fopen(out, "r");
	if (!fp) {
		if (printf("FAIL %s: open failed\n", out) < 0)
			die("test: printf:");
		free(out);
		return true;
	}
	size_t len;
	char *want = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("test: fclose:");
	size_t k = 0;
	for (size_t i = 0; i < len; i++)
		if (want[i] == '\n')
			k++;
	bool failed = false;
	if (k == 0 || want[len - 1] != '\n') {
		if (printf("FAIL %s: empty or no trailing newline\n", out) < 0)
			die("test: printf:");
		failed = true;
	} else {
		for (size_t i = 0; i < bins->gl_pathc; i++)
			failed |= compare(bins->gl_pathv[i], in, want, k);
	}
	free(want);
	free(out);
	return failed;
}

static bool
directory(const char *dir)
{
	glob_t bins, fixtures;
	char *pat = smprintf("bin/%s/*", dir + strlen("tests/"));
	bool failed = false;
	if (!xglob(pat, &bins)) {
		if (printf("FAIL %s: no builds\n", dir) < 0)
			die("test: printf:");
		failed = true;
	}
	free(pat);
	pat = smprintf("%s/*.in", dir);
	size_t n = bins.gl_pathc ? xglob(pat, &fixtures) : 0;
	for (size_t i = 0; i < n; i++)
		failed |= fixture(fixtures.gl_pathv[i], &bins);
	if (n)
		globfree(&fixtures);
	globfree(&bins);
	free(pat);
	return failed;
}

int
main(int argc, [[maybe_unused]] char *argv[])
{
	if (argc != 1)
		die("usage: test");
	glob_t dirs;
	size_t n = xglob("tests/[0-9][0-9][0-9][0-9]/[0-9][0-9]/", &dirs);
	bool failed = false;
	for (size_t i = 0; i < n; i++) {
		dirs.gl_pathv[i][strlen(dirs.gl_pathv[i]) - 1] = '\0';
		failed |= directory(dirs.gl_pathv[i]);
	}
	globfree(&dirs);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
