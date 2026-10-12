/* 2015 day 17, 00 naive: https://adventofcode.com/2015/day/17
 *
 * approach: enumerate subsets; count exact fills and minimal-container fills
 * cost: time O(n * 2^n), memory O(n + input bytes), one input pass
 * trade: direct exhaustive search, limited to fewer than 64 containers
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbit.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static int *
readcaps(const char *p, size_t *n)
{
	int *caps = NULL;

	while (*p) {
		if (isspace((unsigned char)*p)) {
			p++;
			continue;
		}
		errno = 0;
		char *end;
		long cap = strtol(p, &end, 10);
		if (p == end || errno == ERANGE || cap <= 0 || cap > INT_MAX)
			die("invalid container capacity");
		caps = ereallocarray(caps, *n + 1, sizeof(*caps));
		caps[(*n)++] = (int)cap;
		p = end;
	}
	return caps;
}

int
main(void)
{
	size_t len, n = 0;
	char *in = slurp(stdin, &len);
	int *caps = readcaps(in, &n);
	long long ways = 0, minimal = 0;

	if (n >= sizeof(unsigned long long) * CHAR_BIT)
		die("too many containers for subset enumeration");
	unsigned long long limit = 1ULL << n;
	size_t best = n + 1;
	for (unsigned long long mask = 1; mask < limit; mask++) {
		int left = 150;
		unsigned long long bits = mask;
		for (size_t i = 0; bits; bits >>= 1, i++) {
			if (!(bits & 1))
				continue;
			if (caps[i] > left)
				break;
			left -= caps[i];
		}
		if (bits || left)
			continue;
		if (ckd_add(&ways, ways, 1))
			die("too many combinations");
		size_t used = stdc_count_ones(mask);
		if (used < best) {
			best = used;
			minimal = 0;
		}
		if (used == best && ckd_add(&minimal, minimal, 1))
			die("too many minimal combinations");
	}
	if (printf("%lld\n%lld\n", ways, minimal) < 0)
		die("write:");
	free(caps);
	free(in);
	return EXIT_SUCCESS;
}
