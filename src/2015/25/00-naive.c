/* 2015 day 25, 00 naive: https://adventofcode.com/2015/day/25
 *
 * approach: walk the upward diagonals, generating each code until the
 * requested row and column are reached.
 * cost: time O((row + column)^2), memory O(input), one input pass.
 * trade: directly follows the manual rather than skipping earlier codes;
 * day 25 has no second programming part, so print only the requested code.
 */
#include <errno.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	if (!len) {
		free(in);
		return EXIT_SUCCESS;
	}
	char *p = strstr(in, "row ");
	if (p)
		p += 4;
	else
		die("expected row and column");
	errno = 0;
	char *end;
	unsigned long long row = strtoull(p, &end, 10);
	if (errno || end == p || *p == '-' || !row ||
	    strncmp(end, ", column ", 9) != 0)
		die("expected positive row followed by column");
	p = end + 9;
	errno = 0;
	unsigned long long col = strtoull(p, &end, 10);
	if (errno || end == p || *p == '-' || !col)
		die("expected positive row and column");
	unsigned long long r = 1, c = 1, code = 20151125;
	while (r != row || c != col) {
		if (r == 1) {
			if (ckd_add(&r, c, 1))
				die("diagonal overflow");
			c = 1;
		} else {
			r--;
			if (ckd_add(&c, c, 1))
				die("column overflow");
		}
		code = code * 252533 % 33554393;
	}
	if (printf("%llu\n", code) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
