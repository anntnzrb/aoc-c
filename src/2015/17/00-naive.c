/* 2015 day 17, 00 naive: https://adventofcode.com/2015/day/17
 *
 * approach: enumerate subsets of distinct containers; count exact fills of 150
 * cost: time O(n * 2^n), memory O(n + input bytes), one input pass
 * trade: direct exhaustive search, limited to fewer than 64 containers
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

int
main(void)
{
	char *in, *p, *end;
	int *caps = NULL, left;
	long cap;
	size_t len, n = 0, i;
	unsigned long long mask, bits, limit = 0;
	long long ways = 0;

	in = slurp(stdin, &len);
	for (p = in; *p;) {
		if (isspace((unsigned char)*p)) {
			p++;
			continue;
		}
		errno = 0;
		cap = strtol(p, &end, 10);
		if (p == end || errno == ERANGE || cap <= 0 || cap > INT_MAX)
			die("invalid container capacity");
		caps = ereallocarray(caps, n + 1, sizeof(*caps));
		caps[n++] = (int)cap;
		p = end;
	}
	if (n < sizeof(mask) * CHAR_BIT)
		limit = 1ULL << n;
	else
		die("too many containers for subset enumeration");
	for (mask = 1; mask < limit; mask++) {
		left = 150;
		for (bits = mask, i = 0; bits; bits >>= 1, i++) {
			if (!(bits & 1))
				continue;
			if (caps[i] > left)
				break;
			left -= caps[i];
		}
		if (!bits && !left)
			ways++;
	}
	if (printf("%lld\n", ways) < 0)
		die("write:");
	free(caps);
	free(in);
	return EXIT_SUCCESS;
}
