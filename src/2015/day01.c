/* 2015 day 01: https://adventofcode.com/2015/day/1 */
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static long long step(char c);
static long long part1(const char *in, size_t len);
static long long part2(const char *in, size_t len);

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
	size_t i;

	for (i = 0; i < len; i++)
		floor += step(in[i]);
	return floor;
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
