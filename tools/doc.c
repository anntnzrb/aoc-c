/* doc: check that every solution header records its reasoning
 *
 * usage: doc
 *
 * The repo is learning material, so a solution without its reasoning is
 * incomplete. Every src/YEAR/DD/NN-slug.c header comment must contain
 * "approach:" and "cost:", then "bound:" and "rejected:" for 01-optimal
 * (why nothing beats it, what lost) or "trade:" for every other solution.
 * Each tag must open a line of the leading comment (" * cost:"), so code
 * cannot satisfy it and clang-format cannot bury it mid-sentence.
 */
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"
#include "util.h"

static int check(const char *path);

static int
check(const char *path)
{
	static const char *const tags[] = {
	    "approach:", "cost:", "trade:", "bound:", "rejected:"};
	FILE *fp;
	char *s, *end, *needle;
	size_t len, i;
	int failed = 0, optimal, found;

	if (!(fp = fopen(path, "r")))
		die("doc: fopen %s:", path);
	s = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("doc: fclose %s:", path);
	if (strncmp(s, "/*", 2) != 0 || !(end = strstr(s, "*/")))
		end = s;
	*end = '\0';
	/* 01 needs approach, cost, bound, rejected; the rest approach, cost,
	 * trade: skip index 2 for 01 and 3-4 for everything else */
	optimal = !strncmp(strrchr(path, '/') + 1, "01-", 3);
	for (i = 0; i < LEN(tags); i++) {
		if ((optimal && i == 2) || (!optimal && i > 2))
			continue;
		needle = smprintf("\n * %s", tags[i]);
		found = strstr(s, needle) != NULL;
		free(needle);
		if (found)
			continue;
		failed = 1;
		if (printf("FAIL %s: no header line starts with %s\n", path,
		           tags[i]) < 0)
			die("doc: printf:");
	}
	free(s);
	return failed;
}

int
main(void)
{
	glob_t g;
	size_t i, n;
	int failed = 0;

	n = xglob("src/*/*/*.c", &g);
	for (i = 0; i < n; i++)
		failed |= check(g.gl_pathv[i]);
	globfree(&g);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
