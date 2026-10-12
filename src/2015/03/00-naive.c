/* 2015 day 03, 00 naive: https://adventofcode.com/2015/day/3
 *
 * approach: alternate walkers and scan the shared list of visited houses.
 * cost: time O(n^2), memory O(n), two input passes.
 * trade: a plain list makes duplicate detection direct but quadratic.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	long x, y;
} Point;

static size_t
houses(const char *in, size_t len, size_t walkers)
{
	Point pos[2] = {}, *seen = ecalloc(len + 1, sizeof(*seen));
	size_t turn = 0, count = 1;
	for (size_t i = 0; i < len; i++) {
		Point *p = &pos[turn];
		switch (in[i]) {
		case '^':
			p->y++;
			break;
		case 'v':
			p->y--;
			break;
		case '>':
			p->x++;
			break;
		case '<':
			p->x--;
			break;
		case '\n':
			continue;
		default:
			die("invalid direction");
		}
		size_t j = 0;
		for (; j < count; j++)
			if (seen[j].x == p->x && seen[j].y == p->y)
				break;
		if (j == count)
			seen[count++] = *p;
		turn = (turn + 1) % walkers;
	}
	free(seen);
	return count;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	if (len >= LONG_MAX)
		die("input too long");
	if (printf("%zu\n%zu\n", houses(in, len, 1), houses(in, len, 2)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
