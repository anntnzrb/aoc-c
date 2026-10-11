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
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static int nextcap(char **p);
static void solve(char *in, long long *total, long long *minimal);

static int
nextcap(char **p)
{
	char *end;
	long cap;

	while (isspace((unsigned char)**p))
		(*p)++;
	if (!**p)
		return 0;
	errno = 0;
	cap = strtol(*p, &end, 10);
	if (*p == end || errno == ERANGE || cap <= 0 || cap > INT_MAX)
		die("invalid container capacity");
	*p = end;
	return (int)cap;
}

static void
solve(char *in, long long *total, long long *minimal)
{
	long long ways[151] = {1}, fewest[151] = {1};
	int best[151], cap, v, used, top = 0;

	for (v = 1; v <= 150; v++)
		best[v] = 151;
	best[0] = 0;
	while ((cap = nextcap(&in))) {
		if (cap > 150)
			continue;
		top = MIN(150, top + cap);
		/* Descending volumes prevent using this container twice. */
		for (v = top; v >= cap; v--) {
			ways[v] += ways[v - cap];
			used = best[v - cap] + 1;
			if (used < best[v]) {
				best[v] = used;
				fewest[v] = fewest[v - cap];
			} else if (used == best[v]) {
				fewest[v] += fewest[v - cap];
			}
		}
	}
	*total = ways[150];
	*minimal = fewest[150];
}

int
main(void)
{
	char *in;
	size_t len;
	long long total = 0, minimal = 0;

	in = slurp(stdin, &len);
	if (len)
		solve(in, &total, &minimal);
	if (printf("%lld\n%lld\n", total, minimal) < 0)
		die("write:");
	free(in);
	return EXIT_SUCCESS;
}
