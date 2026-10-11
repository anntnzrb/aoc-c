/* 2015 day 09, 00 naive: https://adventofcode.com/2015/day/9
 *
 * approach: try every city order, summing adjacent distances without a
 * return trip, and retain the shortest and longest complete routes.
 * cost: time O(n * n!), memory O(n^2), one input pass.
 * trade: exhaustive search; bounded storage supports 32 cities and names
 * of at most 63 characters, rejecting larger inputs.
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#define CITIES 32

typedef struct {
	char names[CITIES][64];
	long long distance[CITIES][CITIES];
	int used[CITIES];
	size_t n;
} Graph;

static size_t city(Graph *g, const char *name);
static long long route(Graph *g, size_t last, size_t left);

static size_t
city(Graph *g, const char *name)
{
	size_t i;

	for (i = 0; i < g->n; i++)
		if (strcmp(g->names[i], name) == 0)
			return i;
	if (g->n == CITIES)
		die("too many cities");
	memcpy(g->names[g->n], name, strlen(name) + 1);
	return g->n++;
}

static long long
route(Graph *g, size_t last, size_t left)
{
	long long best = LLONG_MAX, tail, distance;
	size_t i;

	if (left == 0)
		return 0;
	for (i = 0; i < g->n; i++) {
		distance = g->distance[last][i];
		if (g->used[i] || distance < 0)
			continue;
		g->used[i] = 1;
		tail = route(g, i, left - 1);
		g->used[i] = 0;
		if (tail == LLONG_MAX)
			continue;
		if (distance >= LLONG_MAX - tail)
			die("route distance overflow");
		best = MIN(best, distance + tail);
	}
	return best;
}

int
main(void)
{
	Graph g = {0};
	char from[64], to[64], *in, *line, *save;
	long long distance, best = LLONG_MAX, candidate;
	size_t len, a, b, i, j;
	int end;

	for (i = 0; i < CITIES; i++)
		for (j = 0; j < CITIES; j++)
			g.distance[i][j] = -1;
	in = slurp(stdin, &len);
	for (line = strtok_r(in, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		end = 0;
		if (sscanf(line, "%63s to %63s = %n", from, to, &end) != 2 ||
		    end == 0)
			die("invalid distance");
		errno = 0;
		distance = strtoll(line + end, &endptr, 10);
		if (errno || endptr == line + end || distance < 0 ||
		    endptr[strspn(endptr, " \t\r")] != 0)
			die("invalid distance");
		a = city(&g, from);
		b = city(&g, to);
		g.distance[a][b] = g.distance[b][a] = distance;
	}
	for (i = 0; i < g.n; i++)
		order[i] = i;
	do {
		total = route(&g, order);
		if (total >= 0) {
			best = MIN(best, total);
			worst = MAX(worst, total);
		}
	} while (next(order, g.n));
	if (best == LLONG_MAX)
		die("no route visits every city");
	if (printf("%lld\n%lld\n", best, worst) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
