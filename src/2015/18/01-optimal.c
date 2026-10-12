/* 2015 day 18, 01 optimal: https://adventofcode.com/2015/day/18
 *
 * approach: pack rows into 64-bit words; balanced Boolean sums count eight
 * neighbors for 64 cells together. Off rows pad the top and bottom; shifts
 * carry across word boundaries. Pin corners initially and after each step.
 * cost: O(width * height + steps * height * ceil(width / 64)) time,
 * O(height * ceil(width / 64)) grid memory, one input pass and one word
 * pass per step for each part; fixed storage covers the 100x100 limit
 * bound: reading input costs Omega(width * height). Dense explicit updates
 * touch every word each step, attaining their word-RAM bound; this is not
 * a proof that predicting only the final population requires every step.
 * rejected: padded bytes work 63529379 (63529379-63529380); serial bit
 * carries 4777823 (4777822-4777824); naive 86950141 (86950141-86950142).
 * Winner work 4457824 (4457824-4457824). just bench 18 50 on Linux x86_64,
 * identical release flags and input; work uses five paired startup-subtracted
 * instruction counts, timing uses 50 runs. Host load makes wall time noisy.
 */
#include <stdbit.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	uint64_t row[102][2];
} Grid;

static void
parse(const char *in, Grid *grid, int *width, int *height)
{
	size_t w = strcspn(in, "\n");
	if (!w || w > 100)
		die("invalid grid width");
	*width = (int)w;
	int y = 0;
	for (const char *p = in; *p;) {
		if (++y > 100)
			die("invalid grid height");
		for (int x = 0; x < *width; x++, p++) {
			if (*p != '.' && *p != '#')
				die("invalid grid row");
			if (*p == '#')
				grid->row[y][x / 64] |= UINT64_C(1)
				                        << (x % 64);
		}
		if (*p && *p != '\n')
			die("invalid grid width");
		if (*p)
			p++;
	}
	*height = y;
}

static uint64_t
shifted(const uint64_t row[2], int word, int words, bool left)
{
	uint64_t bits = row[word];

	if (left)
		return ((bits & (UINT64_MAX >> 1)) << 1) |
		       (word ? row[word - 1] >> 63 : 0);
	return (bits >> 1) |
	       (word + 1 < words ? (row[word + 1] & 1) << 63 : 0);
}

static uint64_t
alive(const uint64_t bits[8], uint64_t old)
{
	uint64_t sa = bits[0] ^ bits[1] ^ bits[2];
	uint64_t a = (bits[0] & bits[1]) | ((bits[0] ^ bits[1]) & bits[2]);
	uint64_t sb = bits[5] ^ bits[6] ^ bits[7];
	uint64_t b = (bits[5] & bits[6]) | ((bits[5] ^ bits[6]) & bits[7]);
	uint64_t sc = bits[3] ^ bits[4], c = bits[3] & bits[4];
	uint64_t ones = sa ^ sb ^ sc;
	uint64_t d = (sa & sb) | ((sa ^ sb) & sc), twos = a ^ b ^ c ^ d;
	/* Two carries of weight two mean at least four live neighbors. */
	uint64_t high = (a & b) | (c & d) | ((a | b) & (c | d));
	return ~high & twos & (ones | old);
}

static void
evolve(const Grid *grid, Grid *next, int width, int height)
{
	int words = (width + 63) / 64;

	uint64_t mask = UINT64_MAX >> ((64 - (width % 64)) % 64);
	for (int y = 1; y <= height; y++) {
		for (int word = 0; word < words; word++) {
			uint64_t bits[8];
			bits[0] = shifted(grid->row[y - 1], word, words, 1);
			bits[1] = grid->row[y - 1][word];
			bits[2] = shifted(grid->row[y - 1], word, words, 0);
			bits[3] = shifted(grid->row[y], word, words, 1);
			bits[4] = shifted(grid->row[y], word, words, 0);
			bits[5] = shifted(grid->row[y + 1], word, words, 1);
			bits[6] = grid->row[y + 1][word];
			bits[7] = shifted(grid->row[y + 1], word, words, 0);
			next->row[y][word] = alive(bits, grid->row[y][word]);
		}
		next->row[y][words - 1] &= mask;
	}
}

static void
corners(Grid *grid, int width, int height)
{
	int word = (width - 1) / 64;
	uint64_t bit = UINT64_C(1) << ((width - 1) % 64);

	grid->row[1][0] |= 1;
	grid->row[1][word] |= bit;
	grid->row[height][0] |= 1;
	grid->row[height][word] |= bit;
}

static long long
simulate(const Grid *initial, int width, int height, long steps, bool stuck)
{
	Grid a = *initial, b = {}, *grid = &a, *next = &b;

	if (stuck)
		corners(grid, width, height);
	for (long step = 0; step < steps; step++) {
		evolve(grid, next, width, height);
		if (stuck)
			corners(next, width, height);
		Grid *swap = grid;
		grid = next;
		next = swap;
	}
	long long total = 0;
	int words = (width + 63) / 64;
	for (int y = 1; y <= height; y++)
		for (int word = 0; word < words; word++)
			total += stdc_count_ones(grid->row[y][word]);
	return total;
}

int
main(void)
{
	Grid grid = {};
	size_t len;
	char *in = slurp(stdin, &len);
	long long p1 = 0, p2 = 0;
	if (len) {
		long steps = 100;
		const char *p = in;
		if (strncmp(p, "steps ", 6) == 0) {
			char *end;
			steps = strtol(p + 6, &end, 10);
			if (steps < 0 || steps > 10000 || *end != '\n')
				die("invalid step count");
			p = end + 1;
		}
		int width, height;
		parse(p, &grid, &width, &height);
		p1 = simulate(&grid, width, height, steps, false);
		p2 = simulate(&grid, width, height, steps, true);
	}
	if (printf("%lld\n%lld\n", p1, p2) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
