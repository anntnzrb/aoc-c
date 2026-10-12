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

typedef struct {
	long long low, high;
} Range;

static void
parse(Graph *g)
{
	for (size_t i = 0; i < CITIES; i++)
		for (size_t j = 0; j < CITIES; j++)
			g->distance[i][j] = -1;
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
		size_t a = nameid(g->names, &g->n, CITIES, from);
		size_t b = nameid(g->names, &g->n, CITIES, to);
		g->distance[a][b] = g->distance[b][a] = distance;
	}
	free(in);
}

static Range *
table(size_t n, size_t states)
{
	size_t count;
	if (ckd_mul(&count, states, n))
		die("route table too large");
	Range *dp = ecalloc(count, sizeof(*dp));
	for (size_t i = 0; i < count; i++) {
		dp[i].low = LLONG_MAX;
		dp[i].high = -1;
	}
	for (size_t i = 0; i < n; i++)
		dp[(((size_t)1 << i) * n) + i] = (Range){};
	return dp;
}

static void
extend(Range *dst, Range src, long long distance)
{
	if (src.high < 0 || distance < 0)
		return;
	long long high;
	if (ckd_add(&high, src.high, distance) || high == LLONG_MAX)
		die("route distance overflow");
	dst->low = MIN(dst->low, src.low + distance);
	dst->high = MAX(dst->high, high);
}

static Range
solve(const Graph *g)
{
	Range result = {LLONG_MAX, -1};

	if (g->n == 0) {
		result.low = result.high = 0;
		return result;
	}
	size_t states = (size_t)1 << g->n;
	Range *dp = table(g->n, states);
	for (size_t mask = 1; mask < states; mask++) {
		for (size_t i = 0; i < g->n; i++) {
			Range src = dp[(mask * g->n) + i];
			if (src.high < 0)
				continue;
			for (size_t j = 0; j < g->n; j++) {
				size_t bit = (size_t)1 << j;
				long long distance = g->distance[i][j];
				if ((mask & bit) || distance < 0)
					continue;
				Range *dst = &dp[((mask | bit) * g->n) + j];
				extend(dst, src, distance);
			}
		}
	}
	for (size_t i = 0; i < g->n; i++) {
		Range src = dp[((states - 1) * g->n) + i];
		result.low = MIN(result.low, src.low);
		result.high = MAX(result.high, src.high);
	}
	free(dp);
	return result;
}

int
main(void)
{
	Graph g = {};

	parse(&g);
	Range result = solve(&g);
	if (result.high < 0)
		die("no route visits every city");
	if (printf("%lld\n%lld\n", result.low, result.high) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
