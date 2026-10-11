/* build: compile solutions into OUTROOT/YEAR/DD/NN-slug
 *
 * usage: build OUTROOT YEAR DAY CC [ARG...]
 *
 * YEAR and DAY select the sources in src/YEAR/DD, or every solution under src
 * when both are "all". Each source is compiled as CC ARG... -o OUT SRC; flags
 * come from the justfile, which stays the only place they are chosen. A failed
 * compile is reported and the rest still build.
 */
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"
#include "util.h"

static int build(const char *root, char *src, int argc, char **args);

/* src/YEAR/DD/NN-slug.c -> ROOT/YEAR/DD/NN-slug */
static int
build(const char *root, char *src, int argc, char **args)
{
	char o[] = "-o", *out, *cmd[128];
	int i, failed;

	out = smprintf("%s/%.*s", root, (int)strlen(src) - 6, src + 4);
	*strrchr(out, '/') = '\0';
	mkdirp(out);
	out[strlen(out)] = '/';
	if (argc > (int)LEN(cmd) - 4)
		die("build: too many compiler arguments");
	for (i = 0; i < argc; i++)
		cmd[i] = args[i];
	cmd[i] = o;
	cmd[i + 1] = out;
	cmd[i + 2] = src;
	cmd[i + 3] = NULL;
	failed = reap(spawn(cmd, -1, -1, -1));
	if (printf(failed ? "FAIL build %s\n" : "%s\n", failed ? src : out) <
	    0)
		die("build: printf:");
	free(out);
	return failed;
}

int
main(int argc, char *argv[])
{
	glob_t g;
	const char *err;
	char *pat;
	size_t i, n;
	int year, day, all, failed = 0;

	if (argc < 5)
		die("usage: build OUTROOT YEAR DAY CC [ARG...]");
	all = !strcmp(argv[2], "all") && !strcmp(argv[3], "all");
	if (all)
		pat = smprintf("src/*/*/*.c");
	else if ((err = puzzleday(argv[2], argv[3], &year, &day)))
		die("build: %s %s: %s", argv[2], argv[3], err);
	else
		pat = smprintf("src/%d/%02d/*.c", year, day);
	if (!(n = xglob(pat, &g)) && !all)
		die("build: no solutions match %s", pat);
	for (i = 0; i < n; i++)
		failed |= build(argv[1], g.gl_pathv[i], argc - 4, argv + 4);
	globfree(&g);
	free(pat);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
