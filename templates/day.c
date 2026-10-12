/* YEAR day DAY, 00 naive: https://adventofcode.com/YEAR/day/DAYN
 *
 * approach: TODO the idea, starting from what the puzzle asks
 * cost: time O(?), memory O(?), passes ?
 * trade: the first thing that works; no optimization
 */
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static long long
part1(const char *in, size_t len)
{
	(void)in;
	(void)len;
	return 0;
}

static long long
part2(const char *in, size_t len)
{
	(void)in;
	(void)len;
	return 0;
}

int
main(void)
{
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	printf("%lld\n%lld\n", part1(in, len), part2(in, len));
	free(in);
	return EXIT_SUCCESS;
}
