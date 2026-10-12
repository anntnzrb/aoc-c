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

static bool
visit(House *seen, size_t mask, Point p, unsigned int flag)
{
	uint64_t x = (uint64_t)p.x, y = (uint64_t)p.y;
	uint64_t h = x ^ (((y & UINT32_MAX) << 32) | (y >> 32));
	h ^= h >> 17;
	h ^= h >> 31;
	/* A 32-bit multiplicand keeps this product within uint64_t. */
	h = (h & UINT32_MAX) * UINT64_C(2654435761);
	size_t i = (size_t)(h ^ (h >> 32)) & mask;
	while (seen[i].flags && (seen[i].p.x != p.x || seen[i].p.y != p.y))
		i = (i + 1) & mask;
	if (seen[i].flags & flag)
		return false;
	seen[i].p = p;
	seen[i].flags |= flag;
	return true;
}

static void
solve(const char *in, size_t len, size_t *one, size_t *two)
{
	if (len >= LONG_MAX || len >= SIZE_MAX / 8)
		die("input too long");
	/* At most 2n+1 houses; leave at least half the slots empty. */
	size_t cap = 1;
	while (cap < 4 * (len + 1))
		cap *= 2;
	House *seen = ecalloc(cap, sizeof(*seen));
	Point solo = {}, pair[2] = {};
	*one = visit(seen, cap - 1, solo, 1);
	*two = visit(seen, cap - 1, solo, 2);
	size_t turn = 0;
	for (size_t i = 0; i < len; i++) {
		char c = in[i];
		if (c == '\n')
			continue;
		if (c != '^' && c != 'v' && c != '>' && c != '<')
			die("invalid direction");
		int dx = (c == '>') - (c == '<');
		int dy = (c == '^') - (c == 'v');
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
	size_t len;
	char *in = slurp(stdin, &len);
	size_t one, two;
	solve(in, len, &one, &two);
	if (printf("%zu\n%zu\n", one, two) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
