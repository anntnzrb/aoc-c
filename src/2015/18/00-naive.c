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

static int neighbors(const unsigned char *grid, int width, int height, int x,
                     int y);
static void evolve(const unsigned char *grid, unsigned char *next, int width,
                   int height);
static int rows(const char *in, int width);
static void corners(unsigned char *grid, int width, int height);
static long long simulate(const char *in, size_t len, int stuck);

static int
neighbors(const unsigned char *grid, int width, int height, int x, int y)
{
	int dx, dy, nx, ny, count = 0;
	size_t i;

	for (dy = -1; dy <= 1; dy++) {
		for (dx = -1; dx <= 1; dx++) {
			nx = x + dx;
			ny = y + dy;
			if ((dx || dy) && nx >= 0 && nx < width && ny >= 0 &&
			    ny < height) {
				i = ((size_t)ny * (size_t)width) + (size_t)nx;
				count += grid[i];
			}
		}
	}
	return count;
}

static void
evolve(const unsigned char *grid, unsigned char *next, int width, int height)
{
	int x, y, count;
	size_t i;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			count = neighbors(grid, width, height, x, y);
			i = ((size_t)y * (size_t)width) + (size_t)x;
			next[i] = count == 3 || (grid[i] && count == 2);
		}
	}
}

static int
rows(const char *in, int width)
{
	const char *p;
	int x, height = 0;

	for (p = in; *p;) {
		for (x = 0; x < width; x++, p++)
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
simulate(const char *in, size_t len, int stuck)
{
	unsigned char *grid, *next, *swap;
	const char *p;
	char *end;
	long steps = 100, step;
	long long total = 0;
	int width = 0, height;
	size_t i, cells;

	if (!len)
		return 0;
	if (strncmp(in, "steps ", 6) == 0) {
		steps = strtol(in + 6, &end, 10);
		if (steps < 0 || steps > 10000 || *end != '\n')
			die("invalid step count");
		in = end + 1;
	}
	for (p = in; *p && *p != '\n'; p++)
		width++;
	if (!width || width > 100)
		die("invalid grid width");
	height = rows(in, width);
	cells = (size_t)width * (size_t)height;
	grid = ecalloc(cells, sizeof(*grid));
	next = ecalloc(cells, sizeof(*next));
	for (p = in, i = 0; *p; p++)
		if (*p != '\n')
			grid[i++] = *p == '#';
	if (stuck)
		corners(grid, width, height);
	for (step = 0; step < steps; step++) {
		evolve(grid, next, width, height);
		if (stuck)
			corners(next, width, height);
		swap = grid;
		grid = next;
		next = swap;
	}
	for (i = 0; i < cells; i++)
		total += grid[i];
	free(next);
	free(grid);
	return total;
}

int
main(void)
{
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	if (printf("%lld\n%lld\n", simulate(in, len, 0),
	           simulate(in, len, 1)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
