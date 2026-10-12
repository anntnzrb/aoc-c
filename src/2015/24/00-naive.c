/* 2015 day 24, 00 naive: https://adventofcode.com/2015/day/24
 *
 * approach: enumerate first groups by size, minimize their product, and
 * backtrack over assignments of the remaining packages to equal groups.
 * cost: time O(n * g^n) for g groups, memory O(n), one input pass.
 * trade: exhaustive search with explicit stacks and sum/product pruning.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	unsigned long long chosen, product;
	int next, left, sum;
} Frame;

static int weights[63], n, target;
static unsigned long long best;

static bool
partition(unsigned long long mask, int groups)
{
	int packages[63], choices[63] = {}, sums[3] = {};
	int count = 0;
	for (int i = 0; i < n; i++)
		if (mask & (1ULL << i))
			packages[count++] = weights[i];
	int i = 0;
	while (i >= 0) {
		if (i == count)
			return true;
		int group = choices[i];
		for (; group < groups; group++)
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
	return false;
}

static void
first(int count, unsigned long long mask, int groups)
{
	Frame stack[64];
	stack[0] = (Frame){0, 1, 0, count, 0};
	int depth = 0;
	while (depth >= 0) {
		Frame *frame = &stack[depth];
		if (!frame->left) {
			if (frame->sum == target && frame->product < best &&
			    partition(mask ^ frame->chosen, groups - 1))
				best = frame->product;
			depth--;
			continue;
		}
		if (frame->next > n - frame->left) {
			depth--;
			continue;
		}
		int i = frame->next++;
		if (weights[i] <= target - frame->sum &&
		    frame->product <= best / (unsigned int)weights[i])
			stack[++depth] = (Frame){
			    frame->chosen | (1ULL << i),
			    frame->product * (unsigned int)weights[i], i + 1,
			    frame->left - 1, frame->sum + weights[i]};
	}
}

static unsigned long long
solve(int sum, int groups)
{
	if (sum % groups)
		die("total weight is not divisible by group count");
	target = sum / groups;
	unsigned long long mask = (1ULL << n) - 1;
	for (int count = 1; count <= n; count++) {
		best = ULLONG_MAX;
		first(count, mask, groups);
		if (best != ULLONG_MAX)
			return best;
	}
	die("no balanced partition with a representable product");
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len), *p = in;
	int sum = 0;
	while (*p) {
		if (isspace((unsigned char)*p)) {
			p++;
			continue;
		}
		errno = 0;
		char *end;
		long value = strtol(p, &end, 10);
		if (errno || end == p || n == 63 || value <= 0 ||
		    ckd_add(&sum, sum, value))
			die("invalid package weight or too many packages");
		weights[n++] = (int)value;
		p = end;
	}
	free(in);
	if (!n)
		return EXIT_SUCCESS;
	unsigned long long part1 = solve(sum, 3), part2 = solve(sum, 4);
	if (printf("%llu\n%llu\n", part1, part2) < 0)
		die("write answer:");
	return EXIT_SUCCESS;
}
