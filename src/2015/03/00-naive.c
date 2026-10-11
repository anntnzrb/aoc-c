/* 2015 day 03, 00 naive: https://adventofcode.com/2015/day/3
 *
 * approach: walk the directions and scan previous houses before adding one.
 * cost: time O(n^2), memory O(n), one input pass.
 * trade: a plain list makes duplicate detection direct but quadratic.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	long x, y;
} Point;

static size_t houses(const char *in, size_t len);

static size_t
houses(const char *in, size_t len)
{
	Point *seen, p = {0, 0};
	size_t i, j, count = 1;

	seen = ecalloc(len + 1, sizeof(*seen));
	for (i = 0; i < len; i++) {
		switch (in[i]) {
		case '^':
			p.y++;
			break;
		case 'v':
			p.y--;
			break;
		case '>':
			p.x++;
			break;
		case '<':
			p.x--;
			break;
		case '\n':
			continue;
		default:
			die("invalid direction");
		}
		for (j = 0; j < count; j++)
			if (seen[j].x == p.x && seen[j].y == p.y)
				break;
		if (j == count)
			seen[count++] = p;
	}
	free(seen);
	return count;
}

int
main(void)
{
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	if (len >= LONG_MAX)
		die("input too long");
	if (printf("%zu\n", houses(in, len)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
