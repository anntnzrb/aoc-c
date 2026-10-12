/* 2015 day 17, 01 optimal: https://adventofcode.com/2015/day/17
 *
 * approach: fold each container into volume counts and minimum-count ways
 * cost: time O(B + n * T), memory O(B + T), one pass; B = bytes, T = 150
 * bound: fixed T gives linear time, matching Omega(input bytes) to read input
 * minimum counts suffice: a larger-count prefix cannot improve an extension
 * rejected: volume/count DP used 173749 work (173749-173750, n=5); this
 * volume-only DP used 35080 (35079-35080). just bench 17 50, Linux x86_64;
 * naive used 104358147 (104358146-104358147).
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static int
nextcap(char **p)
{
	while (isspace((unsigned char)**p))
		(*p)++;
	if (!**p)
		return 0;
	errno = 0;
	char *end;
	long cap = strtol(*p, &end, 10);
	if (*p == end || errno == ERANGE || cap <= 0 || cap > INT_MAX)
		die("invalid container capacity");
	*p = end;
	return (int)cap;
}

static void
fold(int cap, int top, long long *ways, long long *fewest, int *best,
     bool bounded)
{
	/* Descending volumes prevent using this container twice. */
	for (int v = top; v >= cap; v--) {
		if (bounded)
			ways[v] += ways[v - cap];
		else if (ckd_add(&ways[v], ways[v], ways[v - cap]))
			die("too many combinations");
		int used = best[v - cap] + 1;
		if (used < best[v]) {
			best[v] = used;
			fewest[v] = fewest[v - cap];
		} else if (used == best[v]) {
			if (bounded)
				fewest[v] += fewest[v - cap];
			else if (ckd_add(&fewest[v], fewest[v],
			                 fewest[v - cap]))
				die("too many minimal combinations");
		}
	}
}

static void
solve(char *in, long long *total, long long *minimal)
{
	long long ways[151] = {1}, fewest[151] = {1};
	int best[151], cap, top = 0;
	unsigned count = 0;

	for (int v = 1; v <= 150; v++)
		best[v] = 151;
	best[0] = 0;
	while ((cap = nextcap(&in))) {
		if (cap > 150)
			continue;
		top = MIN(150, top + cap);
		/* At most 62 usable containers give <= 2^62 subsets. */
		if (count < 62) {
			count++;
			fold(cap, top, ways, fewest, best, true);
		} else {
			fold(cap, top, ways, fewest, best, false);
		}
	}
	*total = ways[150];
	*minimal = fewest[150];
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	long long total = 0, minimal = 0;

	if (len)
		solve(in, &total, &minimal);
	if (printf("%lld\n%lld\n", total, minimal) < 0)
		die("write:");
	free(in);
	return EXIT_SUCCESS;
}
