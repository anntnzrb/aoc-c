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
#include <unistd.h>

#include "aoc.h"
#include "util.h"

/* drop every earlier output of the selected days, so a deleted or renamed
 * solution cannot linger in bin/ and get tested or benched by mistake */
static void
clean(const char *root, const char *pat)
{
	glob_t g;
	char *outs =
	    pat ? smprintf("%s/%.*s*", root, (int)strlen(pat) - 7, pat + 4)
	        : smprintf("%s/[0-9][0-9][0-9][0-9]/[0-9][0-9]/*", root);
	size_t n = xglob(outs, &g);
	for (size_t i = 0; i < n; i++)
		if (unlink(g.gl_pathv[i]) < 0)
			die("build: unlink %s:", g.gl_pathv[i]);
	globfree(&g);
	free(outs);
}

/* src/YEAR/DD/NN-slug.c -> ROOT/YEAR/DD/NN-slug */
static bool
build(const char *root, char *src, int argc, char **args)
{
	char *out = smprintf("%s/%.*s", root, (int)strlen(src) - 6, src + 4);
	*strrchr(out, '/') = '\0';
	mkdirp(out);
	out[strlen(out)] = '/';
	char o[] = "-o", *cmd[128];
	if (argc > (int)LEN(cmd) - 4)
		die("build: too many compiler arguments");
	int i = 0;
	for (; i < argc; i++)
		cmd[i] = args[i];
	cmd[i] = o;
	cmd[i + 1] = out;
	cmd[i + 2] = src;
	cmd[i + 3] = NULL;
	bool failed = reap(spawn(cmd, -1, -1, -1));
	if (printf(failed ? "FAIL build %s\n" : "%s\n", failed ? src : out) <
	    0)
		die("build: printf:");
	free(out);
	return failed;
}

int
main(int argc, char *argv[])
{
	if (argc < 5)
		die("usage: build OUTROOT YEAR DAY CC [ARG...]");
	bool all = strcmp(argv[2], "all") == 0 && strcmp(argv[3], "all") == 0;
	char *pat;
	if (all)
		pat = smprintf("src/*/*/*.c");
	else {
		int year, day;
		const char *err = puzzleday(argv[2], argv[3], &year, &day);
		if (err)
			die("build: %s %s: %s", argv[2], argv[3], err);
		pat = smprintf("src/%d/%02d/*.c", year, day);
	}
	glob_t g;
	size_t n = xglob(pat, &g);
	if (!n && !all)
		die("build: no solutions match %s", pat);
	clean(argv[1], all ? NULL : pat);
	bool failed = false;
	for (size_t i = 0; i < n; i++)
		failed |= build(argv[1], g.gl_pathv[i], argc - 4, argv + 4);
	globfree(&g);
	free(pat);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
