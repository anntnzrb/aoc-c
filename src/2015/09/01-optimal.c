/* 2015 day 09, subset DP: https://adventofcode.com/2015/day/9
 *
 * approach: routes sharing visited cities and endpoint have the same
 * possible extensions; retain only their shortest and longest lengths.
 * cost: time O(n^2 * 2^n), memory O(n * 2^n), one input pass.
 * bound: reading pair distances costs Omega(n^2). Exact arbitrary-weight
 * paths encode Hamiltonian path; no matching exponential bound is known.
 * rejected: before extraction, backward DP used 230069 work instructions
 * versus 229796 forward. With shared helpers: forward 231574; 00 6642315
 * (Linux just bench 9 50). Both DPs have the same asymptotic cost.
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

typedef struct {
	long long low, high;
} Range;

static void parse(Graph *g);
static Range *table(size_t n, size_t states);
static void extend(Range *dst, Range src, long long distance);
static Range solve(const Graph *g);

static void
parse(Graph *g)
{
	char from[64], to[64], *in, *line, *save, *endptr;
	long long distance;
	size_t len, a, b, i, j;
	int end;

	for (i = 0; i < CITIES; i++)
		for (j = 0; j < CITIES; j++)
			g->distance[i][j] = -1;
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
		a = nameid(g->names, &g->n, CITIES, from);
		b = nameid(g->names, &g->n, CITIES, to);
		g->distance[a][b] = g->distance[b][a] = distance;
	}
	free(in);
}

static Range *
table(size_t n, size_t states)
{
	Range *dp;
	size_t i;

	dp = ecalloc(states, n * sizeof(*dp));
	for (i = 0; i < states * n; i++) {
		dp[i].low = LLONG_MAX;
		dp[i].high = -1;
	}
	for (i = 0; i < n; i++)
		dp[(((size_t)1 << i) * n) + i] = (Range){0, 0};
	return dp;
}

static void
extend(Range *dst, Range src, long long distance)
{
	if (src.high < 0 || distance < 0)
		return;
	if (distance >= LLONG_MAX - src.high)
		die("route distance overflow");
	dst->low = MIN(dst->low, src.low + distance);
	dst->high = MAX(dst->high, src.high + distance);
}

static Range
solve(const Graph *g)
{
	Range *dp, *dst, src, result = {LLONG_MAX, -1};
	long long distance;
	size_t states, mask, i, j, bit;

	if (g->n == 0) {
		result.low = result.high = 0;
		return result;
	}
	states = (size_t)1 << g->n;
	dp = table(g->n, states);
	for (mask = 1; mask < states; mask++) {
		for (i = 0; i < g->n; i++) {
			src = dp[(mask * g->n) + i];
			if (src.high < 0)
				continue;
			for (j = 0; j < g->n; j++) {
				bit = (size_t)1 << j;
				distance = g->distance[i][j];
				if ((mask & bit) || distance < 0)
					continue;
				dst = &dp[((mask | bit) * g->n) + j];
				extend(dst, src, distance);
			}
		}
	}
	for (i = 0; i < g->n; i++) {
		src = dp[((states - 1) * g->n) + i];
		result.low = MIN(result.low, src.low);
		result.high = MAX(result.high, src.high);
	}
	free(dp);
	return result;
}

int
main(void)
{
	Graph g = {0};
	Range result;

	parse(&g);
	result = solve(&g);
	if (result.high < 0)
		die("no route visits every city");
	if (printf("%lld\n%lld\n", result.low, result.high) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
