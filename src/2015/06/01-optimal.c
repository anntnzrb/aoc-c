/* 2015 day 06, 01 optimal: https://adventofcode.com/2015/day/6
 *
 * approach: compress both axes at rectangle boundaries, then apply each
 * command once to the weighted grid. Hoist operations out of cell loops.
 * cost: O(N + K + A + X Y) time, O(N + K + X Y) memory; one input and
 * command pass, A updated compressed cells, X and Y axis bands
 * bound: Omega(N) input; X,Y <= min(1000, 2K + 1). Dense simulation
 * needs Omega(A + X Y) cell visits; global optimality is unproved.
 * rejected: row-band tree 86965738 work; dense rows 67315973 work;
 * two-axis compressed rows 32195975 work (just bench 6 50, Linux).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	unsigned int x1, y1, x2, y2;
	int op;
} Instruction;

static unsigned int coordinate(const char **p, const char *suffix);
static Instruction *instructions(const char *in, size_t len, size_t *count,
                                 unsigned char edges[1001]);
static void apply(unsigned char lights[1000], unsigned int bright[1000],
                  const Instruction *cmd);

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

static Instruction *
instructions(const char *in, size_t len, size_t *count,
             unsigned char edges[1001])
{
	Instruction *list, *cmd;
	const char *p;

	/* The shortest valid instruction bounds the command count. */
	list =
	    len ? ecalloc((len / (sizeof("toggle 0,0 through 0,0") - 1)) + 1,
	                  sizeof(*list))
	        : NULL;
	*count = 0;
	edges[0] = edges[1000] = 1;
	for (p = in; *p;) {

		cmd = &list[(*count)++];
		if (!strncmp(p, "turn on ", 8)) {
			cmd->op = 1;
			p += 8;
		} else if (!strncmp(p, "turn off ", 9)) {
			cmd->op = 0;
			p += 9;
		} else if (!strncmp(p, "toggle ", 7)) {
			cmd->op = 2;
			p += 7;
		} else
			die("invalid instruction");
		cmd->x1 = coordinate(&p, ",");
		cmd->y1 = coordinate(&p, " through ");
		cmd->x2 = coordinate(&p, ",") + 1;
		cmd->y2 = coordinate(&p, "") + 1;
		if ((*p && *p != '\n') || cmd->x1 >= cmd->x2 ||
		    cmd->y1 >= cmd->y2)
			die("invalid rectangle");
		edges[cmd->y1] = edges[cmd->y2] = 1;
		if (*p == '\n')
			p++;
	}
	return list;
}

static void
apply(unsigned char lights[1000], unsigned int bright[1000],
      const Instruction *cmd)
{
	unsigned int x;

	if (cmd->op == 0) {
		for (x = cmd->x1; x < cmd->x2; x++) {
			lights[x] = 0;
			bright[x] -= bright[x] > 0;
		}
	} else if (cmd->op == 1) {
		for (x = cmd->x1; x < cmd->x2; x++) {
			lights[x] = 1;
			bright[x]++;
		}
	} else {
		for (x = cmd->x1; x < cmd->x2; x++) {
			lights[x] ^= 1;
			bright[x] += 2;
		}
	}
}

int
main(void)
{
	unsigned char edges[1001] = {0}, xedges[1001] = {0}, *lights;
	unsigned int *bright, y, x, xs[1001], ys[1001];
	unsigned int map[1001] = {0}, ymap[1001] = {0};
	unsigned long long area, count, brightness;
	size_t nx, ny, width, cells;
	Instruction *list;
	char *in;
	size_t len, n, i;

	in = slurp(stdin, &len);
	if (!len) {
		free(in);
		if (printf("0\n0\n") < 0)
			die("printf:");
		return EXIT_SUCCESS;
	}
	list = instructions(in, len, &n, edges);
	free(in);
	xedges[0] = xedges[1000] = 1;
	for (i = 0; i < n; i++)
		xedges[list[i].x1] = xedges[list[i].x2] = 1;
	nx = 0;
	for (x = 0; x <= 1000; x++)
		if (xedges[x]) {
			map[x] = (unsigned int)nx;
			xs[nx++] = x;
		}
	ny = 0;
	for (y = 0; y <= 1000; y++)
		if (edges[y]) {
			ymap[y] = (unsigned int)ny;
			ys[ny++] = y;
		}
	for (i = 0; i < n; i++) {
		list[i].y1 = ymap[list[i].y1];
		list[i].y2 = ymap[list[i].y2];
		list[i].x1 = map[list[i].x1];
		list[i].x2 = map[list[i].x2];
	}
	width = nx - 1;
	cells = width * (ny - 1);
	lights = n ? ecalloc(cells, sizeof(*lights)) : NULL;
	bright = n ? ecalloc(cells, sizeof(*bright)) : NULL;
	for (i = 0; i < n; i++)
		for (y = list[i].y1; y < list[i].y2; y++)
			apply(lights + (y * width), bright + (y * width),
			      &list[i]);
	count = brightness = 0;
	for (y = 0; n && y + 1 < ny; y++)
		for (x = 0; x + 1 < nx; x++) {
			area = (unsigned long long)(xs[x + 1] - xs[x]) *
			       (ys[y + 1] - ys[y]);
			count += lights[(y * width) + x] * area;
			brightness += bright[(y * width) + x] * area;
		}
	free(bright);
	free(lights);
	free(list);
	if (printf("%llu\n%llu\n", count, brightness) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
