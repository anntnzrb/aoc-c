/* 2015 day 01, 00 naive: https://adventofcode.com/2015/day/1
 *
 * approach: the template shape. slurp() the input, then part1 sums the
 * steps and part2 scans again until the floor first reaches -1.
 * cost: time O(n), memory O(n), 2 passes.
 * trade: the most direct reading of the puzzle, one function per part, at
 * the price of holding the input and reading it twice. part1's switch
 * loop is branch-free, so it still vectorizes and beats a naive fused
 * loop.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

/* '(' goes up a floor, ')' down; anything else (a newline) stays put */
static long long
step(char c)
{
	switch (c) {
	case '(':
		return 1;
	case ')':
		return -1;
	default:
		return 0;
	}
}

/* the floor the instructions end on */
static long long
part1(const char *in, size_t len)
{
	long long floor = 0;
	for (size_t i = 0; i < len; i++)
		floor += step(in[i]);
	return floor;
}

/* 1-based position of the first step into the basement; 0 if never */
static long long
part2(const char *in, size_t len)
{
	long long floor = 0;
	for (size_t i = 0; i < len; i++)
		if ((floor += step(in[i])) == -1)
			return (long long)i + 1;
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
