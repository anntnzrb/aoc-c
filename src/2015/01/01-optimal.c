/* 2015 day 01, 01 optimal: https://adventofcode.com/2015/day/1
 *
 * approach: the floor is a running sum ('(' = +1, ')' = -1), so part 1 is
 * the final sum and part 2 the first position where the sum hits -1.
 * Both come out of one streaming pass: scalar while hunting for the
 * basement, then a branch-free sum over the rest.
 * cost: time O(n), memory O(1) (one 64 KiB buffer), 1 pass.
 * bound: part 1 depends on every byte, so every byte must be read once;
 * O(n) time, 1 pass and O(1) memory cannot be beaten.
 * rejected: slurp + part1 + part2 (00-naive.c): O(n) memory and 2
 * passes, 87,844 instructions of work vs 74,363 here. One loop that
 * checks the basement on every byte: also 1 pass, but the branch blocks
 * vectorization and it measured about 20,000 instructions more than
 * 00-naive.
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static constexpr size_t CHUNK = 65536;

int
main(void)
{
	static char buf[CHUNK];
	long long floor = 0, basement = 0, pos = 0;
	size_t n;

	while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
		long long end;
		if (ckd_add(&end, pos, n))
			die("input too long");
		size_t i = 0;
		/* scalar only until the basement is found ... */
		for (; i < n && !basement; i++)
			if ((floor += (buf[i] == '(') - (buf[i] == ')')) == -1)
				basement = pos + (long long)i + 1;
		/* ... then a branch-free sum the compiler can vectorize */
		for (; i < n; i++)
			floor += (buf[i] == '(') - (buf[i] == ')');
		pos = end;
	}
	if (ferror(stdin))
		die("fread:");
	if (printf("%lld\n%lld\n", floor, basement) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
