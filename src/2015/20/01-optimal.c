/* 2015 day 20, 01 optimal: https://adventofcode.com/2015/day/20
 *
 * approach: divisor sums give elf totals. A linear sieve builds part 1
 *           from coprime products and prime powers; part 2 delivers to at
 *           most 50 multiples and tests each house once its elves finish.
 * cost: O(B) time and memory per part (50 visits is fixed), one prefix
 *       pass per part; B is the smaller of ceil(target / rate) and a
 *       qualifying multiple of lcm(1..10).
 * bound: the linear sieve generates each composite once; enumerating H
 *        houses costs Omega(H). The witness bound may exceed H: this is
 *        linear prefix processing, not a global search lower-bound proof.
 * rejected: bench 20 20 on Linux: early-stop delivery sieves 224332276
 *           work, naive 422174463; linear plus capped sieve 73974151.
 *           The winner uses 16472 KiB versus early-stop 8792 KiB.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t linear(size_t target);
static size_t streetbound(size_t target, int limited);
static size_t solve(size_t target, int limited);

static size_t
linear(size_t target)
{
	size_t *sigma, *power, *primes, bound, n, p, next, i, count = 0;

	target = (target / 10) + (target % 10 != 0);
	bound = streetbound(target, 0);
	sigma = ecalloc(bound + 1, sizeof(*sigma));
	power = ecalloc(bound + 1, sizeof(*power));
	primes = ecalloc(bound + 1, sizeof(*primes));
	sigma[1] = 1;
	for (n = 2; n <= bound && sigma[n - 1] < target; n++) {
		if (!power[n]) {
			primes[count++] = n;
			power[n] = n;
			sigma[n] = n + 1;
		}
		for (i = 0; i < count && primes[i] <= bound / n; i++) {
			p = primes[i];
			next = n * p;
			if (n % p == 0) {
				power[next] = power[n] * p;
				sigma[next] = sigma[n] + (power[next] *
				                          sigma[n / power[n]]);
				break;
			}
			power[next] = p;
			sigma[next] = sigma[n] * (p + 1);
		}
	}
	free(primes);
	free(power);
	free(sigma);
	return n - 1;
}

static size_t
solve(size_t target, bool limited)
{
	size_t rate = limited ? 11 : 10;
	target = (target / rate) + (target % rate != 0);
	size_t bound = streetbound(target, limited);
	size_t *presents = ecalloc(bound + 1, sizeof(*presents));
	size_t elf;
	for (elf = 1; elf <= bound; elf++) {
		for (size_t house = elf, visit = 1;
		     house <= bound && (!limited || visit <= 50);
		     house += elf, visit++)
			presents[house] += elf;
		if (presents[elf] >= target)
			break;
	}
	free(presents);
	return elf;
}

int
main(void)
{
	size_t len, part1 = 0, part2 = 0;
	char *in = slurp(stdin, &len);

	if (len) {
		errno = 0;
		char *end;
		size_t target = strtoul(in, &end, 10);
		if (errno || end == in || !target || *in == '-' ||
		    (*end && *end != '\n'))
			die("invalid present target");
		part1 = linear(target);
		part2 = solve(target, true);
	}
	if (printf("%zu\n%zu\n", part1, part2) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
