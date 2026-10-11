/* 2015 day 06, 00 naive: https://adventofcode.com/2015/day/6
 *
 * approach: apply each rectangle to a dark grid, then count lit cells.
 * Repeat for brightness: decrement to a floor of zero; toggle adds two.
 * cost: O(N + A + G) time, O(N + G) memory, two input and grid passes;
 * N input bytes, A total rectangle area, G = 1000 * 1000 cells
 * trade: one integer per light; simulation follows instructions directly
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static unsigned int coordinate(const char **p, const char *suffix);
static void apply(unsigned int *cell, int op, int brightness);
static unsigned long long solve(const char *in, int brightness);

static unsigned int
coordinate(const char **p, const char *suffix)
{
	unsigned int n;
	size_t len;

	if (**p < '0' || **p > '9')
		die("expected coordinate");
	n = 0;
	while (**p >= '0' && **p <= '9') {
		n = (n * 10) + (unsigned int)(**p - '0');
		if (n >= 1000)
			die("coordinate outside grid");
		(*p)++;
	}
	len = strlen(suffix);
	if (strncmp(*p, suffix, len) != 0)
		die("invalid coordinate separator");
	*p += len;
	return n;
}

static void
apply(unsigned int *cell, int op, int brightness)
{
	if (brightness) {
		if (op == 0)
			*cell -= *cell > 0;
		else
			*cell += (unsigned int)op;
	} else if (op == 2)
		*cell ^= 1;
	else
		*cell = (unsigned int)op;
}

static unsigned long long
solve(const char *in, int brightness)
{
	const char *p;
	unsigned int *lights;
	unsigned int x1, y1, x2, y2, x, y;
	unsigned long long count;
	size_t i;
	int op;

	if (!*in)
		return 0;
	lights = ecalloc(1000000, sizeof(*lights));
	for (p = in; *p;) {
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
		x1 = coordinate(&p, ",");
		y1 = coordinate(&p, " through ");
		x2 = coordinate(&p, ",");
		y2 = coordinate(&p, "");
		if ((*p && *p != '\n') || x1 > x2 || y1 > y2)
			die("invalid rectangle");
		for (y = y1; y <= y2; y++)
			for (x = x1; x <= x2; x++)
				apply(&lights[(y * 1000) + x], op, brightness);
		if (*p == '\n')
			p++;
	}
	count = 0;
	for (i = 0; i < 1000000; i++)
		count += lights[i];
	free(lights);
	return count;
}

int
main(void)
{
	char *in;
	size_t len;
	unsigned long long count, brightness;

	in = slurp(stdin, &len);
	count = solve(in, 0);
	brightness = solve(in, 1);
	free(in);
	if (printf("%llu\n%llu\n", count, brightness) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
