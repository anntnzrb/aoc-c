/* 2015 day 13, 01 optimal: https://adventofcode.com/2015/day/13
 *
 * approach: combine reciprocal happiness, then maximize paths from guest 0
 * through each subset of the others. Close a full path for part 1; join two
 * complementary paths at guest 0 for part 2, with ourselves at the ends.
 * cost: O(L + n^2 * 2^(n-1)) time, O(n * 2^(n-1)) memory, one input pass
 * bound: reading L bytes and arbitrary pair weights costs Omega(L + n^2).
 * Exact maximum Hamiltonian cycles are NP-hard; this subset DP avoids
 * factorial enumeration, but a matching exponential lower bound is unknown.
 * rejected: before extraction, dual-table DP used 366381 work; split DP
 * with sscanf 274379. Shared-helper direct-parser split DP uses 93359
 * versus 7985422 for 00 (Linux just bench 13 50). Losers were discarded.
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#define GUESTS 32

typedef struct {
	char names[GUESTS][64];
	long long happiness[GUESTS][GUESTS];
	size_t count;
} Table;

static long long extend(const Table *table, const long long *row,
                        size_t previous, size_t last);
static void solve(const Table *table, long long *part1, long long *part2);
static char *split(char *name, int delimiter);
static void parse(Table *table, char *line);

static long long
extend(const Table *table, const long long *row, size_t previous, size_t last)
{
	long long best = LLONG_MIN, value;
	size_t k;

	for (k = 0; k < table->count - 1; k++) {
		if (!(previous & ((size_t)1 << k)))
			continue;
		value = row[k] + table->happiness[k + 1][last + 1];
		best = MAX(best, value);
	}
	return best;
}

static void
solve(const Table *table, long long *part1, long long *part2)
{
	long long *dp, *best, total;
	size_t m, states, mask, previous, j, bit;

	m = table->count - 1;
	states = (size_t)1 << m;
	dp = ecalloc(states, m * sizeof(*dp));
	best = ecalloc(states, sizeof(*best));
	for (mask = 1; mask < states; mask++) {
		best[mask] = LLONG_MIN;
		for (j = 0; j < m; j++) {
			bit = (size_t)1 << j;
			if (!(mask & bit))
				continue;
			previous = mask ^ bit;
			total = table->happiness[0][j + 1];
			if (previous)
				total = extend(table, dp + (previous * m),
				               previous, j);
			dp[(mask * m) + j] = total;
			best[mask] = MAX(best[mask], total);
		}
	}
	*part1 = LLONG_MIN;
	for (j = 0; j < m; j++) {
		total =
		    dp[((states - 1) * m) + j] + table->happiness[j + 1][0];
		*part1 = MAX(*part1, total);
	}
	*part2 = LLONG_MIN;
	/* Complementary subsets describe the same path in reverse. */
	for (mask = 0; mask < states / 2; mask++) {
		total = best[mask] + best[(states - 1) ^ mask];
		*part2 = MAX(*part2, total);
	}
	free(best);
	free(dp);
}

static char *
split(char *name, int delimiter)
{
	char *end;

	end = strchr(name, delimiter);
	if (end && end != name && (size_t)(end - name) <= 63) {
		*end = '\0';
		return end + 1;
	}
	die("invalid guest name");
}

static void
parse(Table *table, char *line)
{
	static const char middle[] = " happiness units by sitting next to ";
	char *end, *from, *to, *p;
	int sign;
	size_t a, b;
	long amount;

	from = line;
	p = split(from, ' ');
	if (strncmp(p, "would ", 6) != 0)
		die("invalid happiness statement");
	p += 6;
	if (strncmp(p, "gain ", 5) == 0)
		sign = 1;
	else if (strncmp(p, "lose ", 5) == 0)
		sign = -1;
	else
		die("invalid happiness sign");
	p += 5;
	errno = 0;
	amount = strtol(p, &end, 10);
	if (errno || end == p || amount < 0 || amount > INT_MAX)
		die("invalid happiness amount");
	if (strncmp(end, middle, sizeof(middle) - 1) != 0)
		die("invalid happiness statement");
	to = end + sizeof(middle) - 1;
	p = split(to, '.');
	if (*p && strcmp(p, "\n") != 0)
		die("invalid happiness statement");
	a = nameid(table->names, &table->count, GUESTS, from);
	b = nameid(table->names, &table->count, GUESTS, to);
	table->happiness[a][b] = sign * amount;
}

int
main(void)
{
	Table table = {0};
	char *line = NULL;
	size_t capacity = 0, a, b;
	long long answer1 = 0, answer2 = 0, pair;

	while (getline(&line, &capacity, stdin) != -1)
		parse(&table, line);
	if (ferror(stdin))
		die("getline:");
	free(line);
	if (table.count) {
		if (table.count < 3)
			die("need at least three guests");
		for (a = 0; a < table.count; a++)
			for (b = a + 1; b < table.count; b++) {
				pair = table.happiness[a][b] +
				       table.happiness[b][a];
				table.happiness[a][b] = pair;
				table.happiness[b][a] = pair;
			}
		solve(&table, &answer1, &answer2);
	}
	if (printf("%lld\n%lld\n", answer1, answer2) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
