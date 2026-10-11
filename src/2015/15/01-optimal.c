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

static long long score(const long long *base, const long long *slope,
                       long long x);
static int interval(const long long *base, const long long *slope,
                    long long *low, long long *high);
static long long peak(const long long *base, const long long *slope,
                      long long low, long long high);
static long long calorie(const long long *base, const long long *slope,
                         long long low, long long high);
static void pair(const long long *base, const long long *slope, int left,
                 long long best[2]);
static void search(const Ingredient *in, size_t n, long long best[2]);

static long long
score(const long long *base, const long long *slope, long long x)
{
	return (base[0] + (x * slope[0])) * (base[1] + (x * slope[1])) *
	       (base[2] + (x * slope[2])) * (base[3] + (x * slope[3]));
}

static int
interval(const long long *base, const long long *slope, long long *low,
         long long *high)
{
	size_t j;

	/* Skip zero plateaus before comparing adjacent positive scores. */
	for (j = 0; j < 4; j++) {
		if (slope[j] > 0 && base[j] <= 0)
			*low = MAX(*low, (-base[j] / slope[j]) + 1);
		if (slope[j] <= 0 && base[j] <= 0)
			return 0;
		if (slope[j] < 0)
			*high = MIN(*high, (base[j] - 1) / -slope[j]);
	}
	return *low <= *high;
}

static long long
peak(const long long *base, const long long *slope, long long low,
     long long high)
{
	long long mid;

	while (low < high) {
		mid = low + ((high - low) / 2);
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
	long long x;

	/* high+1 means every residual allocation has 500 calories. */
	if (!slope[4])
		return base[4] == 500 ? high + 1 : -1;
	if ((500 - base[4]) % slope[4])
		return -1;
	x = (500 - base[4]) / slope[4];
	return x >= low && x <= high ? x : -1;
}

static void
pair(const long long *base, const long long *slope, int left,
     long long best[2])
{
	long long low = 0, high = left, x, value, upper = 1;
	size_t j;

	if (!interval(base, slope, &low, &high))
		return;
	for (j = 0; j < 4; j++)
		upper *= base[j] + (slope[j] * (slope[j] > 0 ? high : low));
	x = calorie(base, slope, low, high);
	if (upper <= best[0] && (upper <= best[1] || x < 0))
		return;
	value = peak(base, slope, low, high);
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
	State *state;
	long long base[5], slope[5];
	size_t at = 0, depth, j;
	int q;

	depth = n > 1 ? n - 2 : 0;
	state = ecalloc(n, sizeof(*state));
	state[0].left = 100;
	for (j = 0; j < LEN(slope); j++)
		slope[j] = in[depth].prop[j] - in[n - 1].prop[j];
	for (;;) {
		if (at == depth) {
			for (j = 0; j < LEN(base); j++)
				base[j] = state[at].sum[j] +
				          (state[at].left * in[n - 1].prop[j]);
			pair(base, slope, state[at].left, best);
		} else if (state[at].next <= state[at].left) {
			q = state[at].next++;
			for (j = 0; j < LEN(base); j++)
				state[at + 1].sum[j] =
				    state[at].sum[j] + (q * in[at].prop[j]);
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
	char *text, *line, *save, *end;
	const char *p;
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
	if (n)
		search(in, n, best);
	if (printf("%lld\n%lld\n", best[0], best[1]) < 0)
		die("printf:");
	free(in);
	free(text);
	return EXIT_SUCCESS;
}
