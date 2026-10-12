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
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static constexpr size_t CITIES = 32;

typedef struct {
	char names[CITIES][64];
	long long distance[CITIES][CITIES];
	size_t n;
} Graph;

static long long
route(const Graph *g, const size_t *order)
{
	long long total = 0;

	for (size_t i = 1; i < g->n; i++) {
		long long distance = g->distance[order[i - 1]][order[i]];
		if (distance < 0)
			return -1;
		if (ckd_add(&total, total, distance) || total == LLONG_MAX)
			die("route distance overflow");
	}
	return total;
}

int
main(void)
{
	Graph g = {};

	for (size_t i = 0; i < CITIES; i++)
		for (size_t j = 0; j < CITIES; j++)
			g.distance[i][j] = -1;
	size_t len;
	char *in = slurp(stdin, &len), *line, *save;
	for (line = strtok_r(in, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		char from[64], to[64];
		int end = 0;
		if (sscanf(line, "%63s to %63s = %n", from, to, &end) != 2 ||
		    end == 0)
			die("invalid distance");
		errno = 0;
		char *endptr;
		long long distance = strtoll(line + end, &endptr, 10);
		if (errno || endptr == line + end || distance < 0 ||
		    endptr[strspn(endptr, " \t\r")] != 0)
			die("invalid distance");
		size_t a = nameid(g.names, &g.n, CITIES, from);
		size_t b = nameid(g.names, &g.n, CITIES, to);
		g.distance[a][b] = g.distance[b][a] = distance;
	}
	size_t order[CITIES];
	for (size_t i = 0; i < g.n; i++)
		order[i] = i;
	long long best = LLONG_MAX, worst = -1;
	do {
		long long total = route(&g, order);
		if (total >= 0) {
			best = MIN(best, total);
			worst = MAX(worst, total);
		}
	} while (nextperm(order, g.n));
	if (best == LLONG_MAX)
		die("no route visits every city");
	if (printf("%lld\n%lld\n", best, worst) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
