/* 2015 day 15, 01 optimal: https://adventofcode.com/2015/day/15
 *
 * approach: enumerate prefixes; prune and binary-search residual scores,
 *           solving the last two ingredients calorie equation directly
 * cost: O(n + binomial(T+n-2,n-2) * (n + log(T))) time,
 *       O(n) memory, one input pass; T=100, n>=2
 * bound: reading needs Omega(n). Concave logs of positive affine factors
 *        make residual scores unimodal, saving one enumeration dimension.
 *        Independent property maxima safely prune; no matching global
 *        lower bound is known.
 * rejected: Linux, just bench 15 50, work instructions (n=5):
 *           plain binary 1170705 (1170704-1170705),
 *           linear sweep 1363040 (1363040-1363041),
 *           naive 27576793 (27576793-27576794).
 *           Monolithic pruning 1070312 (1070311-1070313) failed the
 *           cognitive complexity limit (56 versus 25). Split pruning
 *           retained with 1072568 (1072568-1072569).
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

typedef struct {
	long long sum[5];
	int left, next;
} State;

static long long
magnitude(long long value)
{
	if (value < -LLONG_MAX / 300 || value > LLONG_MAX / 300)
		die("property out of range");
	return value < 0 ? -value : value;
}

static long long
affine(long long base, long long scale, long long value)
{
	/* Parsed magnitudes <= LLONG_MAX/300 bound affine intermediates. */
	return base + (scale * value);
}

static long long
subtract(long long left, long long right)
{
	return left - right;
}

static long long
score(const long long *base, const long long *slope, long long x)
{
	long long product = 1;
	for (size_t j = 0; j < 4; j++)
		product *= affine(base[j], x, slope[j]);
	return product;
}

static bool
interval(const long long *base, const long long *slope, long long *low,
         long long *high)
{
	/* Skip zero plateaus before comparing adjacent positive scores. */
	for (size_t j = 0; j < 4; j++) {
		if (slope[j] > 0 && base[j] <= 0)
			*low =
			    MAX(*low,
			        affine(1, 1, subtract(0, base[j]) / slope[j]));
		if (slope[j] <= 0 && base[j] <= 0)
			return false;
		if (slope[j] < 0)
			*high =
			    MIN(*high, (base[j] - 1) / subtract(0, slope[j]));
	}
	return *low <= *high;
}

static long long
peak(const long long *base, const long long *slope, long long low,
     long long high)
{
	while (low < high) {
		long long mid = low + ((high - low) / 2);
		if (score(base, slope, mid) < score(base, slope, mid + 1))
			low = mid + 1;
		else
			high = mid;
	}
	return score(base, slope, low);
}

static long long
calorie(const long long *base, const long long *slope, long long low,
        long long high)
{
	/* high+1 means every residual allocation has 500 calories. */
	if (!slope[4])
		return base[4] == 500 ? high + 1 : -1;
	long long remaining = subtract(500, base[4]);
	if (remaining % slope[4])
		return -1;
	long long x = remaining / slope[4];
	return x >= low && x <= high ? x : -1;
}

static void
pair(const long long *base, const long long *slope, int left,
     long long best[2])
{
	long long low = 0, high = left;
	if (!interval(base, slope, &low, &high))
		return;
	long long upper = 1;
	for (size_t j = 0; j < 4; j++) {
		long long value =
		    affine(base[j], slope[j], slope[j] > 0 ? high : low);
		upper *= value;
	}
	long long x = calorie(base, slope, low, high);
	if (upper <= best[0] && (upper <= best[1] || x < 0))
		return;
	long long value = peak(base, slope, low, high);
	best[0] = MAX(best[0], value);
	if (x >= 0) {
		if (x <= high)
			value = score(base, slope, x);
		best[1] = MAX(best[1], value);
	}
}

static void
search(const Ingredient *in, size_t n, long long best[2])
{
	size_t depth = n > 1 ? n - 2 : 0;
	State *state = ecalloc(n, sizeof(*state));
	state[0].left = 100;
	long long slope[5];
	for (size_t j = 0; j < LEN(slope); j++)
		slope[j] = subtract(in[depth].prop[j], in[n - 1].prop[j]);
	size_t at = 0;
	for (;;) {
		if (at == depth) {
			long long base[5];
			for (size_t j = 0; j < LEN(base); j++)
				base[j] =
				    affine(state[at].sum[j], state[at].left,
				           in[n - 1].prop[j]);
			pair(base, slope, state[at].left, best);
		} else if (state[at].next <= state[at].left) {
			int q = state[at].next++;
			for (size_t j = 0; j < LEN(slope); j++)
				state[at + 1].sum[j] = affine(
				    state[at].sum[j], q, in[at].prop[j]);
			state[at + 1].left = state[at].left - q;
			state[++at].next = 0;
			continue;
		}
		if (!at)
			break;
		at--;
	}
	free(state);
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
	long long best[2] = {};
	if (n)
		search(in, n, best);
	if (printf("%lld\n%lld\n", best[0], best[1]) < 0)
		die("printf:");
	free(in);
	free(text);
	return EXIT_SUCCESS;
}
