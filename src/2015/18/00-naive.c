/* 2015 day 18, 00 naive: https://adventofcode.com/2015/day/18
 *
 * approach: count eight neighbors in the old grid, write each next state
 * into a second grid, then swap; part 2 pins corners before and after each
 * step. Fixtures may prefix the grid with steps N.
 * cost: time O(steps * width * height), memory O(width * height), two input
 * passes and one grid pass per step
 * trade: direct simulation with bounds checks for every neighbor
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static int
neighbors(const unsigned char *grid, int width, int height, int x, int y)
{
	int count = 0;

	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			int nx = x + dx, ny = y + dy;
			if ((dx || dy) && nx >= 0 && nx < width && ny >= 0 &&
			    ny < height) {
				size_t i =
				    ((size_t)ny * (size_t)width) + (size_t)nx;
				count += grid[i];
			}
		}
	}
	return count;
}

static void
evolve(const unsigned char *grid, unsigned char *next, int width, int height)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int count = neighbors(grid, width, height, x, y);
			size_t i = ((size_t)y * (size_t)width) + (size_t)x;
			next[i] = count == 3 || (grid[i] && count == 2);
		}
	}
}

static int
rows(const char *in, int width)
{
	int height = 0;

	for (const char *p = in; *p;) {
		for (int x = 0; x < width; x++, p++)
			if (*p != '.' && *p != '#')
				die("invalid grid row");
		if (*p && *p != '\n')
			die("invalid grid width");
		if (*p)
			p++;
		height++;
	}
	if (!height || height > 100)
		die("invalid grid height");
	return height;
}

static void
corners(unsigned char *grid, int width, int height)
{
	size_t lastrow = (size_t)(height - 1) * (size_t)width;

	grid[0] = 1;
	grid[width - 1] = 1;
	grid[lastrow] = 1;
	grid[lastrow + (size_t)(width - 1)] = 1;
}

static long long
simulate(const char *in, size_t len, bool stuck)
{
	if (!len)
		return 0;
	/* Grid <= 100 x 100 plus prefix: <= 10112 bytes, counts fit in int. */
	if (len > 10112)
		die("grid too large");
	long steps = 100;
	if (strncmp(in, "steps ", 6) == 0) {
		char *end;
		steps = strtol(in + 6, &end, 10);
		if (steps < 0 || steps > 10000 || *end != '\n')
			die("invalid step count");
		in = end + 1;
	}
	int width = 0;
	for (const char *p = in; *p && *p != '\n'; p++)
		width++;
	if (!width || width > 100)
		die("invalid grid width");
	int height = rows(in, width);
	size_t cells = (size_t)width * (size_t)height, i = 0;
	unsigned char *grid = ecalloc(cells, sizeof(*grid));
	unsigned char *next = ecalloc(cells, sizeof(*next));
	for (const char *p = in; *p; p++)
		if (*p != '\n')
			grid[i++] = *p == '#';
	if (stuck)
		corners(grid, width, height);
	for (long step = 0; step < steps; step++) {
		evolve(grid, next, width, height);
		if (stuck)
			corners(next, width, height);
		unsigned char *swap = grid;
		grid = next;
		next = swap;
	}
	long long total = 0;
	for (size_t cell = 0; cell < cells; cell++)
		total += grid[cell];
	free(next);
	free(grid);
	return total;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);

	if (printf("%lld\n%lld\n", simulate(in, len, false),
	           simulate(in, len, true)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
