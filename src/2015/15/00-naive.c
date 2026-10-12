/* 2015 day 15, 00 naive: https://adventofcode.com/2015/day/15
 *
 * approach: enumerate all allocations of 100 teaspoons and score each recipe,
 *           keeping a second maximum for recipes with exactly 500 calories
 * cost: O(n * binomial(100+n-1,n-1)) time, O(n) memory, one input pass
 * trade: exhaustive search is simple and sufficient for a few ingredients
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	long long prop[5];
} Ingredient;

static long long
magnitude(long long value)
{
	if (value < -LLONG_MAX / 300 || value > LLONG_MAX / 300)
		die("property out of range");
	return value < 0 ? -value : value;
}

static long long
total(const Ingredient *in, const int *amount, size_t n, size_t property)
{
	long long sum = 0;
	/* Amounts sum to 100; parsed absolute property bounds fit. */
	for (size_t i = 0; i < n; i++)
		sum += amount[i] * in[i].prop[property];
	return sum;
}

static long long
score(const Ingredient *in, const int *amount, size_t n)
{
	long long product = 1;
	for (size_t j = 0; j < 4; j++) {
		long long sum = total(in, amount, n, j);
		product *= MAX(sum, 0);
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
	long long limit[5] = {};
	size_t len;
	char *text = slurp(stdin, &len), *save;
	size_t n = 0;
	for (char *line = strtok_r(text, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		const char *p = strchr(line, ':');
		if (!p)
			die("invalid ingredient");
		in = ereallocarray(in, n + 1, sizeof(*in));
		for (size_t j = 0; j < LEN(in[n].prop); j++) {
			while (*p && *p != '-' && !isdigit((unsigned char)*p))
				p++;
			char *end;
			errno = 0;
			in[n].prop[j] = strtoll(p, &end, 10);
			if (p == end || errno)
				die("invalid property");
			long long value = magnitude(in[n].prop[j]);
			limit[j] = MAX(limit[j], value);
			p = end;
		}
		n++;
	}
	/* Every property is bounded by 100*limit; all score prefixes fit. */
	long long bound = 1;
	for (size_t j = 0; j < 4; j++)
		if (ckd_mul(&bound, bound, limit[j] * 100))
			die("score bound overflow");
	int *amount = ecalloc(n, sizeof(*amount));
	long long best[2] = {};
	if (n)
		search(in, amount, n, best);
	if (printf("%lld\n%lld\n", best[0], best[1]) < 0)
		die("printf:");
	free(amount);
	free(in);
	free(text);
	return EXIT_SUCCESS;
}
