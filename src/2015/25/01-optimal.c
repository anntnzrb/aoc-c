/* 2015 day 25, 01 optimal: https://adventofcode.com/2015/day/25
 *
 * approach: diagonal d = row + column - 2 has d(d+1)/2 earlier cells;
 * raise the multiplier to d(d+1)/2 + column - 1 by binary squaring.
 * cost: O(input + log(index)) time, O(input) memory, one input pass.
 * bound: reading input costs Omega(input); generic multiplication-only
 * exponentiation needs Omega(log(index)) steps since each can at most
 * double the largest exponent. No input-sized grid or fixed lookup table.
 * rejected: just bench 25 50 on Linux: right-to-left 3496 work instructions
 * (3496-3497); left-to-right 3539 (3538-3539), despite skipping the leading
 * bit's square. The naive diagonal walk used 374860560 (374860535-374860570).
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

int
main(void)
{
	char *in, *p, *end;
	size_t len;
	unsigned long long row, col, d, a, b, n, base = 252533;
	unsigned long long code = 20151125;

	in = slurp(stdin, &len);
	if (!len) {
		free(in);
		return EXIT_SUCCESS;
	}
	p = strstr(in, "row ");
	if (p)
		p += 4;
	else
		die("expected row and column");
	errno = 0;
	row = strtoull(p, &end, 10);
	if (errno || end == p || *p == '-' || !row ||
	    strncmp(end, ", column ", 9) != 0)
		die("expected positive row followed by column");
	p = end + 9;
	errno = 0;
	col = strtoull(p, &end, 10);
	if (errno || end == p || *p == '-' || !col)
		die("expected positive row and column");
	if (row > ULLONG_MAX - col)
		die("diagonal overflow");
	d = row + col - 2;
	a = d;
	b = d + 1;
	if (a % 2)
		b /= 2;
	else
		a /= 2;
	if (a && b > (ULLONG_MAX - (col - 1)) / a)
		die("code index overflow");
	n = (a * b) + (col - 1);
	while (n) {
		if (n & 1)
			code = code * base % 33554393;
		n >>= 1;
		if (n)
			base = base * base % 33554393;
	}
	if (printf("%llu\n", code) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
