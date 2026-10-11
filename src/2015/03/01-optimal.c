/* 2015 day 03, 01 optimal: https://adventofcode.com/2015/day/3
 *
 * approach: move the solo and alternating walkers together; a shared hash
 * set records which houses each delivery scheme has reached.
 * cost: expected time O(n), worst O(n^2), memory O(n), one input pass.
 * bound: every direction can affect the answer, requiring Omega(n) reads.
 * Hashing meets that time bound in expectation, not in the worst case.
 * rejected: sorting two position arrays used 9964330 work instructions;
 * the quadratic naive scans used 118739610 (just bench 3 50, Linux).
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	long x, y;
} Point;

typedef struct {
	Point p;
	unsigned int flags;
} House;

static size_t visit(House *seen, size_t mask, Point p, unsigned int flag);
static void solve(const char *in, size_t len, size_t *one, size_t *two);

static size_t
visit(House *seen, size_t mask, Point p, unsigned int flag)
{
	uint64_t x, y, h;
	size_t i;

	x = (uint64_t)p.x;
	y = (uint64_t)p.y;
	h = x ^ (((y & UINT32_MAX) << 32) | (y >> 32));
	h ^= h >> 17;
	h ^= h >> 31;
	/* A 32-bit multiplicand keeps this product within uint64_t. */
	h = (h & UINT32_MAX) * UINT64_C(2654435761);
	i = (size_t)(h ^ (h >> 32)) & mask;
	while (seen[i].flags && (seen[i].p.x != p.x || seen[i].p.y != p.y))
		i = (i + 1) & mask;
	if (seen[i].flags & flag)
		return 0;
	seen[i].p = p;
	seen[i].flags |= flag;
	return 1;
}

static void
solve(const char *in, size_t len, size_t *one, size_t *two)
{
	House *seen;
	Point solo = {0, 0}, pair[2] = {{0, 0}, {0, 0}};
	size_t i, cap = 1, turn = 0;
	int dx, dy;
	char c;

	if (len >= LONG_MAX || len >= SIZE_MAX / 8)
		die("input too long");
	/* At most 2n+1 houses; leave at least half the slots empty. */
	while (cap < 4 * (len + 1))
		cap *= 2;
	seen = ecalloc(cap, sizeof(*seen));
	*one = visit(seen, cap - 1, solo, 1);
	*two = visit(seen, cap - 1, solo, 2);
	for (i = 0; i < len; i++) {
		c = in[i];
		if (c == '\n')
			continue;
		if (c != '^' && c != 'v' && c != '>' && c != '<')
			die("invalid direction");
		dx = (c == '>') - (c == '<');
		dy = (c == '^') - (c == 'v');
		solo.x += dx;
		solo.y += dy;
		pair[turn].x += dx;
		pair[turn].y += dy;
		*one += visit(seen, cap - 1, solo, 1);
		*two += visit(seen, cap - 1, pair[turn], 2);
		turn ^= 1;
	}
	free(seen);
}

int
main(void)
{
	char *in;
	size_t len, one, two;

	in = slurp(stdin, &len);
	solve(in, len, &one, &two);
	if (printf("%zu\n%zu\n", one, two) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
