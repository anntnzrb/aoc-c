/* 2015 day 13, 00 naive: https://adventofcode.com/2015/day/13
 *
 * approach: fix one guest, try every order of the others, and sum both
 * peoples happiness on every circular neighbor pair. For part 2, add
 * a guest with zero happiness in both directions and repeat the search.
 * cost: O(n * (n-1)!) time, O(n^2) memory, one input pass
 * trade: exhaustive permutations keep the first solution straightforward;
 * guest names are at most 63 characters, with at most 32 input guests plus
 * ourselves.
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static constexpr size_t GUESTS = 32;

typedef struct {
	char names[GUESTS][64];
	long long happiness[GUESTS + 1][GUESTS + 1];
	size_t count;
} Table;

static long long
arrange(const Table *table)
{
	size_t order[GUESTS + 1];
	for (size_t i = 0; i < table->count; i++)
		order[i] = i;
	long long best = LLONG_MIN;
	do {
		long long total = 0;
		for (size_t i = 0; i < table->count; i++) {
			size_t a = order[i];
			size_t b = order[(i + 1) % table->count];
			total +=
			    table->happiness[a][b] + table->happiness[b][a];
		}
		best = MAX(best, total);
	} while (nextperm(order + 1, table->count - 1));
	return best;
}

int
main(void)
{
	Table table = {};
	char *line = NULL;
	size_t capacity = 0;
	while (getline(&line, &capacity, stdin) != -1) {
		char from[64], to[64], sign[5], number[32], extra;
		if (sscanf(line,
		           "%63s would %4s %31s happiness units by sitting "
		           "next to "
		           "%63[^.]. %c",
		           from, sign, number, to, &extra) != 4 ||
		    (strcmp(sign, "gain") != 0 && strcmp(sign, "lose") != 0))
			die("invalid happiness statement");
		char *end;
		errno = 0;
		long amount = strtol(number, &end, 10);
		if (errno || end == number || *end || amount < 0 ||
		    amount > INT_MAX)
			die("invalid happiness amount");
		size_t a = nameid(table.names, &table.count, GUESTS, from);
		size_t b = nameid(table.names, &table.count, GUESTS, to);
		table.happiness[a][b] =
		    strcmp(sign, "lose") == 0 ? -amount : amount;
	}
	if (ferror(stdin))
		die("getline:");
	free(line);
	long long answer1 = 0, answer2 = 0;
	if (table.count) {
		if (table.count < 3)
			die("need at least three guests");
		answer1 = arrange(&table);
		table.count++;
		answer2 = arrange(&table);
	}
	if (printf("%lld\n%lld\n", answer1, answer2) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
