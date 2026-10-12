/* 2015 day 24, 01 optimal: https://adventofcode.com/2015/day/24
 *
 * approach: sort descending; bound fixed-size first groups by attainable
 * sums, look up the last weight, and partition the remainder while skipping
 * equivalent bucket loads. Minimize count before product.
 * cost: time O(n * g^n) for g groups, memory O(n), one input pass.
 * bound: reading L bytes costs Omega(L). Two-way PARTITION reduces to
 * g-way balance by adding g-2 packages of half the original total, so an
 * exact solver is NP-hard. No matching exponential lower bound is known;
 * these bounds discard impossible sums without discarding feasible groups.
 * rejected: bounded enumeration without final-weight lookup: 453567 work;
 * knapsack optimum with remainder certificate and search fallback: 517774.
 * Final lookup search: 251707; 00: 45160475 (Linux just bench 24 50).
 * Both losing candidates were discarded.
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

static int weights[63], prefix[64], n, target;
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
		for (; group < groups; group++) {
			int other = 0;
			for (; other < group; other++)
				if (sums[other] == sums[group])
					break;
			if (other == group &&
			    packages[i] <= target - sums[group])
				break;
		}
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

static int
lookup(int start, int need)
{
	int low = start, high = n;
	while (low < high) {
		int middle = low + ((high - low) / 2);
		if (weights[middle] > need)
			low = middle + 1;
		else
			high = middle;
	}
	return low;
}

static void
first(int count, unsigned long long mask, int groups)
{
	Frame stack[64];
	stack[0] = (Frame){0, 1, 0, count, 0};
	int depth = 0;
	while (depth >= 0) {
		Frame *frame = &stack[depth];
		if (frame->left == 1) {
			int need = target - frame->sum;
			int low = lookup(frame->next, need);
			if (need > 0 && low < n && weights[low] == need &&
			    frame->product <= best / (unsigned int)need) {
				unsigned long long product =
				    frame->product * (unsigned int)need;
				if (product < best &&
				    partition(
				        mask ^ (frame->chosen | (1ULL << low)),
				        groups - 1))
					best = product;
			}
			depth--;
			continue;
		}
		if (frame->next > n - frame->left ||
		    prefix[frame->next + frame->left] - prefix[frame->next] <
		        target - frame->sum ||
		    prefix[n] - prefix[n - frame->left] >
		        target - frame->sum) {
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
		if (prefix[count] < target)
			continue;
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
	n = 0;
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
	for (int i = 1; i < n; i++) {
		int weight = weights[i], j = i;
		for (; j > 0 && weights[j - 1] < weight; j--)
			weights[j] = weights[j - 1];
		weights[j] = weight;
	}
	for (int i = 0; i < n; i++)
		prefix[i + 1] = prefix[i] + weights[i];
	unsigned long long part1 = solve(sum, 3), part2 = solve(sum, 4);
	if (printf("%llu\n%llu\n", part1, part2) < 0)
		die("write answer:");
	return EXIT_SUCCESS;
}
