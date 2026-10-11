/* 2015 day 24, 00 naive: https://adventofcode.com/2015/day/24
 *
 * approach: enumerate first groups by size, minimize their product, and
 * backtrack over assignments of the remaining packages to equal groups.
 * cost: time O(n * 3^n), memory O(n), one input pass.
 * trade: exhaustive search with explicit stacks and sum/product pruning.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	unsigned long long chosen, product;
	int next, left, sum;
} Frame;

static int partition(unsigned long long mask, int groups);
static void first(int count, unsigned long long mask);
static unsigned long long solve(void);

static int weights[63], n, target;
static unsigned long long best;

static int
partition(unsigned long long mask, int groups)
{
	int packages[63], choices[63] = {0}, sums[2] = {0};
	int count, i, group;

	count = 0;
	for (i = 0; i < n; i++)
		if (mask & (1ULL << i))
			packages[count++] = weights[i];
	i = 0;
	while (i >= 0) {
		if (i == count)
			return 1;
		for (group = choices[i]; group < groups; group++)
			if (packages[i] <= target - sums[group])
				break;
		if (group < groups) {
			choices[i] = group + 1;
			sums[group] += packages[i++];
			if (i < count)
				choices[i] = 0;
		} else if (--i >= 0) {
			sums[choices[i] - 1] -= packages[i];
		}
	}
	return 0;
}

static void
first(int count, unsigned long long mask)
{
	Frame stack[64], *frame;
	int depth, i;

	stack[0] = (Frame){0, 1, 0, count, 0};
	depth = 0;
	while (depth >= 0) {
		frame = &stack[depth];
		if (!frame->left) {
			if (frame->sum == target && frame->product < best &&
			    partition(mask ^ frame->chosen, 2))
				best = frame->product;
			depth--;
			continue;
		}
		if (frame->next > n - frame->left) {
			depth--;
			continue;
		}
		i = frame->next++;
		if (weights[i] <= target - frame->sum &&
		    frame->product <= best / (unsigned int)weights[i])
			stack[++depth] = (Frame){
			    frame->chosen | (1ULL << i),
			    frame->product * (unsigned int)weights[i], i + 1,
			    frame->left - 1, frame->sum + weights[i]};
	}
}

static unsigned long long
solve(void)
{
	unsigned long long mask;
	int count;

	mask = (1ULL << n) - 1;
	for (count = 1; count <= n; count++) {
		best = ULLONG_MAX;
		first(count, mask);
		if (best != ULLONG_MAX)
			return best;
	}
	die("no balanced partition with a representable product");
}

int
main(void)
{
	char *in, *p, *end;
	long value;
	size_t len;
	int sum;

	in = slurp(stdin, &len);
	p = in;
	sum = 0;
	while (*p) {
		if (isspace((unsigned char)*p)) {
			p++;
			continue;
		}
		errno = 0;
		value = strtol(p, &end, 10);
		if (errno || end == p || n == 63 || value <= 0 ||
		    value > INT_MAX - sum)
			die("invalid package weight or too many packages");
		weights[n++] = (int)value;
		sum += (int)value;
		p = end;
	}
	free(in);
	if (!n)
		return EXIT_SUCCESS;
	if (sum % 3)
		die("total weight is not divisible by three");
	target = sum / 3;
	if (printf("%llu\n", solve()) < 0)
		die("write answer:");
	return EXIT_SUCCESS;
}
