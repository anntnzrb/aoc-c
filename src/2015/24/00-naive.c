/* 2015 day 24, 00 naive: https://adventofcode.com/2015/day/24
 *
 * approach: enumerate first groups by size, minimize their product, and
 * recursively split the remainder into equal-weight groups.
 * cost: time O(n * 3^n), memory O(n), one input pass.
 * trade: exhaustive search with sum/product pruning and group symmetry.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static int partition(unsigned long long mask, int groups);
static int fill(unsigned long long mask, int start, int sum,
                unsigned long long chosen, int groups);
static void first(int start, int left, int sum, unsigned long long chosen,
                  unsigned long long product, unsigned long long mask);
static unsigned long long solve(void);

static int weights[63], n, target;
static unsigned long long best;

static int
partition(unsigned long long mask, int groups)
{
	int i;

	if (groups == 1)
		return 1;
	for (i = 0; i < n; i++)
		if (mask & (1ULL << i))
			return fill(mask, i + 1, weights[i], 1ULL << i,
			            groups);
	return 0;
}

static int
fill(unsigned long long mask, int start, int sum, unsigned long long chosen,
     int groups)
{
	int i;

	if (sum == target)
		return partition(mask ^ chosen, groups - 1);
	for (i = start; i < n; i++)
		if ((mask & (1ULL << i)) && weights[i] <= target - sum &&
		    fill(mask, i + 1, sum + weights[i], chosen | (1ULL << i),
		         groups))
			return 1;
	return 0;
}

static void
first(int start, int left, int sum, unsigned long long chosen,
      unsigned long long product, unsigned long long mask)
{
	int i;

	if (!left) {
		if (sum == target && product < best &&
		    partition(mask ^ chosen, 2))
			best = product;
		return;
	}
	for (i = start; i <= n - left; i++)
		if (weights[i] <= target - sum &&
		    product <= best / (unsigned int)weights[i])
			first(i + 1, left - 1, sum + weights[i],
			      chosen | (1ULL << i),
			      product * (unsigned int)weights[i], mask);
}

static unsigned long long
solve(void)
{
	unsigned long long mask;
	int count;

	mask = (1ULL << n) - 1;
	for (count = 1; count <= n; count++) {
		best = ULLONG_MAX;
		first(0, count, 0, 0, 1, mask);
		if (best != ULLONG_MAX)
			return best;
	}
	die("no balanced partition with a representable product");
}

int
main(void)
{
	int value, sum, result;

	sum = 0;
	while ((result = scanf("%d", &value)) == 1) {
		if (n == 63 || value <= 0 || value > INT_MAX - sum)
			die("invalid package weight or too many packages");
		weights[n++] = value;
		sum += value;
	}
	if (result != EOF || ferror(stdin))
		die("read package weights");
	if (!n)
		return EXIT_SUCCESS;
	if (sum % 3)
		die("total weight is not divisible by three");
	target = sum / 3;
	if (printf("%llu\n", solve()) < 0)
		die("write answer:");
	return EXIT_SUCCESS;
}
