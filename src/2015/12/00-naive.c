/* 2015 day 12, 00 naive: https://adventofcode.com/2015/day/12
 *
 * approach: strings contain no numbers, so scan for signed integers and sum.
 * cost: time O(n), memory O(n) for input, one pass.
 * trade: skips JSON structure because part 1 only needs the numbers.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

int
main(void)
{
	char *in, *p, *end;
	long long sum = 0, value;
	size_t len;

	in = slurp(stdin, &len);
	for (p = in; *p;) {
		if (*p != '-' && (*p < '0' || *p > '9')) {
			p++;
			continue;
		}
		errno = 0;
		value = strtoll(p, &end, 10);
		if (errno || end == p)
			die("invalid integer");
		sum += value;
		p = end;
	}
	if (printf("%lld\n", sum) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
