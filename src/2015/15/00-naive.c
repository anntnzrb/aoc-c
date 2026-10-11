/* 2015 day 15, 00 naive: https://adventofcode.com/2015/day/15
 *
 * approach: enumerate all allocations of 100 teaspoons and score each recipe
 * cost: O(n * binomial(100+n-1,n-1)) time, O(n) memory, one input pass
 * trade: exhaustive search is simple and sufficient for a few ingredients
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	long long prop[5];
} Ingredient;

static long long score(const Ingredient *in, const int *amount, size_t n);
static long long search(const Ingredient *in, int *amount, size_t n);

static long long
score(const Ingredient *in, const int *amount, size_t n)
{
	long long total, product = 1;
	size_t i, j;

	for (j = 0; j < 4; j++) {
		total = 0;
		for (i = 0; i < n; i++)
			total += amount[i] * in[i].prop[j];
		product *= MAX(total, 0);
	}
	return product;
}

static long long
search(const Ingredient *in, int *amount, size_t n)
{
	long long best = 0, value;
	int left;
	size_t at, i;

	amount[n - 1] = 100;
	for (;;) {
		value = score(in, amount, n);
		best = MAX(best, value);
		/* Carry the rightmost movable teaspoon to the previous slot.
		 */
		left = amount[n - 1];
		at = n - 1;
		while (at > 0 && !left)
			left += amount[--at];
		if (!at)
			break;
		amount[--at]++;
		for (i = at + 1; i < n - 1; i++)
			amount[i] = 0;
		amount[n - 1] = left - 1;
	}
	return best;
}

int
main(void)
{
	Ingredient *in = NULL;
	char *text, *line, *save, *end;
	const char *p;
	int *amount;
	size_t len, n = 0, j;
	long long best = 0;

	text = slurp(stdin, &len);
	for (line = strtok_r(text, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		if (!(p = strchr(line, ':')))
			die("invalid ingredient");
		in = ereallocarray(in, n + 1, sizeof(*in));
		for (j = 0; j < LEN(in[n].prop); j++) {
			while (*p && *p != '-' && !isdigit((unsigned char)*p))
				p++;
			errno = 0;
			in[n].prop[j] = strtoll(p, &end, 10);
			if (p == end || errno)
				die("invalid property");
			p = end;
		}
		n++;
	}
	amount = ecalloc(n, sizeof(*amount));
	if (n)
		best = search(in, amount, n);
	if (printf("%lld\n", best) < 0)
		die("printf:");
	free(amount);
	free(in);
	free(text);
	return EXIT_SUCCESS;
}
