/* 2015 day 02, 00 naive: https://adventofcode.com/2015/day/2
 *
 * approach: sum twice the three face areas plus the smallest for each box
 * cost: time O(n), memory O(n) input + O(1) working, one pass over n bytes
 * trade: strtoll parses each dimension; the whole input stays in memory
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static long long part1(const char *in, size_t len);
static long long part2(const char *in, size_t len);

static long long
part1(const char *in, size_t len)
{
	long long dim[3], lw, wh, hl, total = 0;
	char *end;
	size_t i;

	(void)len;
	while (*in) {
		for (i = 0; i < LEN(dim); i++) {
			errno = 0;
			dim[i] = strtoll(in, &end, 10);
			if (errno || end == in || dim[i] <= 0 ||
			    *end != (i == 2 ? '\n' : 'x'))
				die("invalid box dimensions");
			in = end + 1;
		}
		lw = dim[0] * dim[1];
		wh = dim[1] * dim[2];
		hl = dim[2] * dim[0];
		total += (2 * (lw + wh + hl)) + MIN(lw, MIN(wh, hl));
	}
	return total;
}

static long long
part2(const char *in, size_t len)
{
	(void)in;
	(void)len;
	return 0;
}

int
main(void)
{
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	if (printf("%lld\n%lld\n", part1(in, len), part2(in, len)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
