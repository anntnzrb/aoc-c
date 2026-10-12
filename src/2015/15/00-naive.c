/* 2015 day 15, 00 naive: https://adventofcode.com/2015/day/15
 *
 * approach: enumerate all allocations of 100 teaspoons and score each recipe,
 *           keeping a second maximum for recipes with exactly 500 calories
 * cost: O(n * binomial(100+n-1,n-1)) time, O(n) memory, one input pass
 * trade: exhaustive search is simple and sufficient for a few ingredients
 */
#include <ctype.h>
#include <errno.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	long long prop[5];
} Ingredient;

static long long
total(const Ingredient *in, const int *amount, size_t n, size_t property)
{
	long long sum = 0;
	for (size_t i = 0; i < n; i++) {
		long long value;
		if (ckd_mul(&value, amount[i], in[i].prop[property]) ||
		    ckd_add(&sum, sum, value))
			die("property overflow");
	}
	return sum;
}

static long long
score(const Ingredient *in, const int *amount, size_t n)
{
	long long product = 1;
	for (size_t j = 0; j < 4; j++) {
		long long sum = total(in, amount, n, j);
		if (ckd_mul(&product, product, MAX(sum, 0)))
			die("score overflow");
	}
	return product;
}

static void
search(const Ingredient *in, int *amount, size_t n, long long best[2])
{
	amount[n - 1] = 100;
	for (;;) {
		long long value = score(in, amount, n);
		best[0] = MAX(best[0], value);
		long long calories = total(in, amount, n, 4);
		if (calories == 500)
			best[1] = MAX(best[1], value);
		/* Carry the rightmost movable teaspoon to the previous slot.
		 */
		int left = amount[n - 1];
		size_t at = n - 1;
		while (at > 0 && !left)
			left += amount[--at];
		if (!at)
			break;
		amount[--at]++;
		for (size_t i = at + 1; i < n - 1; i++)
			amount[i] = 0;
		amount[n - 1] = left - 1;
	}
}

int
main(void)
{
	Ingredient *in = NULL;
	char *text, *line, *save, *end;
	const char *p;
	int *amount;
	size_t len, n = 0, j;
	long long best[2] = {0};

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
		search(in, amount, n, best);
	if (printf("%lld\n%lld\n", best[0], best[1]) < 0)
		die("printf:");
	free(amount);
	free(in);
	free(text);
	return EXIT_SUCCESS;
}
