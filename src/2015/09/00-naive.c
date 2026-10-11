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
	size_t n;
} Graph;

static size_t city(Graph *g, const char *name);
static int next(size_t *order, size_t n);
static long long route(const Graph *g, const size_t *order);

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

static int
next(size_t *order, size_t n)
{
	size_t i, j, tmp;

	if (n < 2)
		return 0;
	for (i = n - 1; i > 0 && order[i - 1] > order[i]; i--)
		;
	if (i == 0)
		return 0;
	for (j = n - 1; order[j] < order[i - 1]; j--)
		;
	tmp = order[i - 1];
	order[i - 1] = order[j];
	order[j] = tmp;
	for (j = n - 1; i < j; i++, j--) {
		tmp = order[i];
		order[i] = order[j];
		order[j] = tmp;
	}
	return 1;
}

static long long
route(const Graph *g, const size_t *order)
{
	long long total = 0, distance;
	size_t i;

	for (i = 1; i < g->n; i++) {
		distance = g->distance[order[i - 1]][order[i]];
		if (distance < 0)
			return -1;
		if (distance >= LLONG_MAX - total)
			die("route distance overflow");
		total += distance;
	}
	return total;
}

int
main(void)
{
	Graph g = {0};
	char from[64], to[64], *in, *line, *save, *endptr;
	long long distance, best = LLONG_MAX, worst = -1, total;
	size_t len, a, b, i, j, order[CITIES];
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
