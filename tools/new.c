/* new: scaffold a solution without overwriting existing work. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"
#include "util.h"

static void
expand(FILE *out, const char *in, int year, int day)
{
	while (*in) {
		int n;
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
	if (argc != 3)
		die("new: usage: new YEAR DAY");
	int year, day;
	const char *err = puzzleday(argv[1], argv[2], &year, &day);
	if (err)
		die("new: %s", err);
	FILE *fp = fopen("templates/day.c", "r");
	if (!fp)
		die("new: open templates/day.c:");
	size_t len;
	char *in = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("new: fclose:");
	char *dir = smprintf("src/%d/%02d", year, day);
	char *tests = smprintf("tests/%d/%02d", year, day);
	char *path = smprintf("%s/00-naive.c", dir);
	mkdirp(dir);
	int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
	if (fd < 0) {
		if (errno == EEXIST)
			die("new: %s exists", path);
		die("new: open %s:", path);
	}
	mkdirp(tests);
	FILE *out = fdopen(fd, "w");
	if (!out)
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
