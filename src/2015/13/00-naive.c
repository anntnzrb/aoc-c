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

#define GUESTS 32

typedef struct {
	char names[GUESTS][64];
	long long happiness[GUESTS + 1][GUESTS + 1];
	size_t count;
} Table;

static size_t guest(Table *table, const char *name);
static int next(size_t *order, size_t count);
static long long arrange(const Table *table);

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

static int
next(size_t *order, size_t count)
{
	size_t i, j, tmp;

	i = count - 1;
	while (i && order[i - 1] >= order[i])
		i--;
	if (!i)
		return 0;
	j = count - 1;
	while (order[j] <= order[i - 1])
		j--;
	tmp = order[i - 1];
	order[i - 1] = order[j];
	order[j] = tmp;
	for (j = count - 1; i < j; i++, j--) {
		tmp = order[i];
		order[i] = order[j];
		order[j] = tmp;
	}
	return 1;
}

static long long
arrange(const Table *table)
{
	long long best = LLONG_MIN, total;
	size_t i, a, b, order[GUESTS + 1];

	for (i = 0; i < table->count; i++)
		order[i] = i;
	do {
		total = 0;
		for (i = 0; i < table->count; i++) {
			a = order[i];
			b = order[(i + 1) % table->count];
			total +=
			    table->happiness[a][b] + table->happiness[b][a];
		}
		best = MAX(best, total);
	} while (next(order + 1, table->count - 1));
	return best;
}

int
main(void)
{
	Table table = {0};
	char *line = NULL, *end;
	char from[64], to[64], sign[5], number[32], extra;
	size_t capacity = 0, a, b;
	long amount;
	long long answer1 = 0, answer2 = 0;

	while (getline(&line, &capacity, stdin) != -1) {
		if (sscanf(line,
		           "%63s would %4s %31s happiness units by sitting "
		           "next to "
		           "%63[^.]. %c",
		           from, sign, number, to, &extra) != 4 ||
		    (strcmp(sign, "gain") != 0 && strcmp(sign, "lose") != 0))
			die("invalid happiness statement");
		errno = 0;
		amount = strtol(number, &end, 10);
		if (errno || end == number || *end || amount < 0 ||
		    amount > INT_MAX)
			die("invalid happiness amount");
		a = guest(&table, from);
		b = guest(&table, to);
		table.happiness[a][b] =
		    !strcmp(sign, "lose") ? -amount : amount;
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
