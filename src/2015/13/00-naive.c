/* 2015 day 13, 00 naive: https://adventofcode.com/2015/day/13
 *
 * approach: fix one guest, try every order of the others, and sum both
 * peoples happiness on every circular neighbor pair.
 * cost: O(n * (n-1)!) time, O(n^2) memory, one input pass
 * trade: exhaustive permutations keep the first solution straightforward;
 * guest names are at most 63 characters, with at most 32 guests.
 */
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

static size_t guest(Table *table, const char *name);
static long long arrange(const Table *table, size_t *order, int *used,
                         size_t depth);

static size_t
guest(Table *table, const char *name)
{
	size_t i;

	for (i = 0; i < table->count; i++)
		if (!strcmp(table->names[i], name))
			return i;
	if (table->count == GUESTS)
		die("too many guests");
	memcpy(table->names[i], name, strlen(name) + 1);
	table->count++;
	return i;
}

static long long
arrange(const Table *table, size_t *order, int *used, size_t depth)
{
	long long best, total;
	size_t i, a, b;

	if (depth == table->count) {
		total = 0;
		for (i = 0; i < table->count; i++) {
			a = order[i];
			b = order[(i + 1) % table->count];
			total +=
			    table->happiness[a][b] + table->happiness[b][a];
		}
		return total;
	}
	best = LLONG_MIN;
	for (i = 1; i < table->count; i++) {
		if (used[i])
			continue;
		used[i] = 1;
		order[depth] = i;
		total = arrange(table, order, used, depth + 1);
		best = MAX(best, total);
		used[i] = 0;
	}
	return best;
}

int
main(void)
{
	Table table = {0};
	char *line = NULL;
	char from[64], to[64], sign[5];
	size_t capacity = 0, a, b, order[GUESTS] = {0};
	int amount, end, used[GUESTS] = {0};
	long long answer = 0;

	while (getline(&line, &capacity, stdin) != -1) {
		end = 0;
		if (sscanf(
		        line,
		        "%63s would %4s %d happiness units by sitting next to "
		        "%63[^.]. %n",
		        from, sign, &amount, to, &end) != 4 ||
		    !end || line[end] || amount < 0 ||
		    (strcmp(sign, "gain") && strcmp(sign, "lose")))
			die("invalid happiness statement");
		a = guest(&table, from);
		b = guest(&table, to);
		table.happiness[a][b] =
		    !strcmp(sign, "lose") ? -(long long)amount : amount;
	}
	if (ferror(stdin))
		die("getline:");
	free(line);
	if (table.count) {
		if (table.count < 3)
			die("need at least three guests");
		answer = arrange(&table, order, used, 1);
	}
	if (printf("%lld\n", answer) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
