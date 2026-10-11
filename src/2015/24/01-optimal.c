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
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	unsigned long long chosen, product;
	int next, left, sum;
} Frame;

static int partition(unsigned long long mask, int groups);
static int lookup(int start, int need);
static void first(int count, unsigned long long mask, int groups);
static unsigned long long solve(int sum, int groups);

static int weights[63], prefix[64], n, target;
static unsigned long long best;

static int
partition(unsigned long long mask, int groups)
{
	int packages[63], choices[63] = {0}, sums[3] = {0};
	int count, i, group, other;

	count = 0;
	for (i = 0; i < n; i++)
		if (mask & (1ULL << i))
			packages[count++] = weights[i];
	i = 0;
	while (i >= 0) {
		if (i == count)
			return 1;
		for (group = choices[i]; group < groups; group++) {
			for (other = 0; other < group; other++)
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
	return 0;
}

static int
lookup(int start, int need)
{
	int low, high, middle;

	low = start;
	high = n;
	while (low < high) {
		middle = low + ((high - low) / 2);
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
	Frame stack[64], *frame;
	int depth, i, low, need;
	unsigned long long product;

	stack[0] = (Frame){0, 1, 0, count, 0};
	depth = 0;
	while (depth >= 0) {
		frame = &stack[depth];
		if (frame->left == 1) {
			need = target - frame->sum;
			low = lookup(frame->next, need);
			if (need > 0 && low < n && weights[low] == need &&
			    frame->product <= best / (unsigned int)need) {
				product = frame->product * (unsigned int)need;
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
solve(int sum, int groups)
{
	unsigned long long mask;
	int count;

	if (sum % groups)
		die("total weight is not divisible by group count");
	target = sum / groups;
	mask = (1ULL << n) - 1;
	for (count = 1; count <= n; count++) {
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
	char *in, *p, *end;
	unsigned long long part1, part2;
	long value;
	size_t len;
	int sum, i, j, weight;

	in = slurp(stdin, &len);
	p = in;
	n = 0;
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
	for (i = 1; i < n; i++) {
		weight = weights[i];
		for (j = i; j > 0 && weights[j - 1] < weight; j--)
			weights[j] = weights[j - 1];
		weights[j] = weight;
	}
	for (i = 0; i < n; i++)
		prefix[i + 1] = prefix[i] + weights[i];
	part1 = solve(sum, 3);
	part2 = solve(sum, 4);
	if (printf("%llu\n%llu\n", part1, part2) < 0)
		die("write answer:");
	return EXIT_SUCCESS;
}
