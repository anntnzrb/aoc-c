/* new: scaffold a solution without overwriting existing work. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"
#include "util.h"

static void expand(FILE *out, const char *in, int year, int day);

static void
expand(FILE *out, const char *in, int year, int day)
{
	int n;

	while (*in) {
		if (strncmp(in, "YEAR", 4) == 0) {
			n = fprintf(out, "%d", year);
			in += 4;
		} else if (strncmp(in, "DAYN", 4) == 0) {
			n = fprintf(out, "%d", day);
			in += 4;
		} else if (strncmp(in, "DAY", 3) == 0) {
			n = fprintf(out, "%02d", day);
			in += 3;
		} else {
			n = fputc((unsigned char)*in++, out);
		}
		if (n < 0)
			die("new: write:");
	}
}

int
main(int argc, char *argv[])
{
	FILE *fp, *out;
	const char *err;
	char *dir, *tests, *path, *in;
	size_t len;
	int year, day, fd;

	if (argc != 3)
		die("new: usage: new YEAR DAY");
	if ((err = puzzleday(argv[1], argv[2], &year, &day)))
		die("new: %s", err);
	if (!(fp = fopen("templates/day.c", "r")))
		die("new: open templates/day.c:");
	in = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("new: fclose:");
	dir = smprintf("src/%d/%02d", year, day);
	tests = smprintf("tests/%d/%02d", year, day);
	path = smprintf("%s/00-naive.c", dir);
	mkdirp(dir);
	if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666)) <
	    0) {
		if (errno == EEXIST)
			die("new: %s exists", path);
		die("new: open %s:", path);
	}
	mkdirp(tests);
	if (!(out = fdopen(fd, "w")))
		die("new: fdopen:");
	expand(out, in, year, day);
	if (fclose(out) == EOF)
		die("new: fclose:");
	if (printf("%s\n", path) < 0)
		die("new: printf:");
	free(in);
	free(path);
	free(tests);
	free(dir);
	return EXIT_SUCCESS;
}
