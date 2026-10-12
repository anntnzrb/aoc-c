/* 2015 day 02, 00 naive: https://adventofcode.com/2015/day/2
 *
 * approach: paper is twice the face areas plus the smallest; ribbon is
 *           the smallest face perimeter plus volume, summed for each box
 * cost: time O(n), memory O(n) input + O(1) working, one pass over n bytes
 * trade: strtoll parses each dimension; the whole input stays in memory
 */
#include <errno.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static long long
add(long long a, long long b)
{
	if (ckd_add(&a, a, b))
		die("answer overflow");
	return a;
}

static long long
mul(long long a, long long b)
{
	if (ckd_mul(&a, a, b))
		die("answer overflow");
	return a;
}

static char *
box(char *p, long long dim[3])
{
	for (size_t i = 0; i < 3; i++) {
		char *end;
		errno = 0;
		dim[i] = strtoll(p, &end, 10);
		if (errno || end == p || dim[i] <= 0 ||
		    *end != (i == 2 ? '\n' : 'x'))
			die("invalid box dimensions");
		p = end + 1;
	}
	return p;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len), *p = in;
	long long paper = 0, ribbon = 0;
	while (*p) {
		long long dim[3];
		p = box(p, dim);
		long long lw = mul(dim[0], dim[1]);
		long long wh = mul(dim[1], dim[2]);
		long long hl = mul(dim[2], dim[0]);
		paper = add(paper, add(mul(2, add(add(lw, wh), hl)),
		                       MIN(lw, MIN(wh, hl))));
		ribbon = add(ribbon, add(mul(2, MIN(add(dim[0], dim[1]),
		                                    MIN(add(dim[1], dim[2]),
		                                        add(dim[2], dim[0])))),
		                         mul(lw, dim[2])));
	}
	if (printf("%lld\n%lld\n", paper, ribbon) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
