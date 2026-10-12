/* 2015 day 06, 00 naive: https://adventofcode.com/2015/day/6
 *
 * approach: apply each rectangle to a dark grid, then count lit cells.
 * Repeat for brightness: decrement to a floor of zero; toggle adds two.
 * cost: O(N + A + G) time, O(N + G) memory, two input and grid passes;
 * N input bytes, A total rectangle area, G = 1000 * 1000 cells
 * trade: one integer per light; simulation follows instructions directly
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static unsigned int
coordinate(const char **p, const char *suffix)
{
	if (**p < '0' || **p > '9')
		die("expected coordinate");
	unsigned int n = 0;
	while (**p >= '0' && **p <= '9') {
		n = (n * 10) + (unsigned int)(**p - '0');
		if (n >= 1000)
			die("coordinate outside grid");
		(*p)++;
	}
	size_t len = strlen(suffix);
	if (strncmp(*p, suffix, len) != 0)
		die("invalid coordinate separator");
	*p += len;
	return n;
}

static void
apply(unsigned int *cell, int op, bool brightness)
{
	if (brightness) {
		if (op == 0)
			*cell -= *cell > 0;
		else if (ckd_add(cell, *cell, (unsigned int)op))
			die("brightness overflow");
	} else if (op == 2)
		*cell ^= 1;
	else
		*cell = (unsigned int)op;
}

static unsigned long long
solve(const char *in, bool brightness)
{
	if (!*in)
		return 0;
	unsigned int *lights = ecalloc(1000000, sizeof(*lights));
	for (const char *p = in; *p;) {
		int op;
		if (!strncmp(p, "turn on ", 8)) {
			op = 1;
			p += 8;
		} else if (!strncmp(p, "turn off ", 9)) {
			op = 0;
			p += 9;
		} else if (!strncmp(p, "toggle ", 7)) {
			op = 2;
			p += 7;
		} else
			die("invalid instruction");
		unsigned int x1 = coordinate(&p, ",");
		unsigned int y1 = coordinate(&p, " through ");
		unsigned int x2 = coordinate(&p, ",");
		unsigned int y2 = coordinate(&p, "");
		if ((*p && *p != '\n') || x1 > x2 || y1 > y2)
			die("invalid rectangle");
		for (unsigned int y = y1; y <= y2; y++)
			for (unsigned int x = x1; x <= x2; x++)
				apply(&lights[(y * 1000) + x], op, brightness);
		if (*p == '\n')
			p++;
	}
	unsigned long long count = 0;
	for (size_t i = 0; i < 1000000; i++)
		count += lights[i];
	free(lights);
	return count;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	unsigned long long count = solve(in, false);
	unsigned long long brightness = solve(in, true);
	free(in);
	if (printf("%llu\n%llu\n", count, brightness) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
