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
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t
streetbound(size_t target, bool limited)
{
	constexpr size_t base = 2520;
	size_t sum = base;

	/* 2520 is lcm(1..10); its divisors remain divisors after scaling. */
	for (size_t divisor = 1; divisor < base; divisor++)
		if (base % divisor == 0 && (!limited || base / divisor <= 50))
			sum += divisor;
	size_t multiple = (target / sum) + (target % sum != 0);
	size_t bound = multiple <= target / base ? multiple * base : target;
	size_t maximum;
	/* Divisor sums and intermediate products are at most bound squared. */
	if (ckd_mul(&maximum, bound, bound))
		die("street bound too large");
	return bound;
}

static size_t
linear(size_t target)
{
	target = (target / 10) + (target % 10 != 0);
	size_t bound = streetbound(target, false);
	size_t *sigma = ecalloc(bound + 1, sizeof(*sigma));
	size_t *power = ecalloc(bound + 1, sizeof(*power));
	size_t *primes = ecalloc(bound + 1, sizeof(*primes));
	sigma[1] = 1;
	size_t n, count = 0;
	for (n = 2; n <= bound && sigma[n - 1] < target; n++) {
		if (!power[n]) {
			primes[count++] = n;
			power[n] = n;
			sigma[n] = n + 1;
		}
		for (size_t i = 0; i < count && primes[i] <= bound / n; i++) {
			size_t p = primes[i], next = n * p;
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
