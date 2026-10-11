/* 2015 day 02, 00 naive: https://adventofcode.com/2015/day/2
 *
 * approach: paper is twice the face areas plus the smallest; ribbon is
 *           the smallest face perimeter plus volume, summed for each box
 * cost: time O(n), memory O(n) input + O(1) working, one pass over n bytes
 * trade: strtoll parses each dimension; the whole input stays in memory
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static char *box(char *p, long long dim[3]);

static char *
box(char *p, long long dim[3])
{
	char *end;
	size_t i;

	for (i = 0; i < 3; i++) {
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
	long long dim[3], lw, wh, hl, paper = 0, ribbon = 0;
	char *in, *p;
	size_t len;

	in = slurp(stdin, &len);
	p = in;
	while (*p) {
		p = box(p, dim);
		lw = dim[0] * dim[1];
		wh = dim[1] * dim[2];
		hl = dim[2] * dim[0];
		paper += (2 * (lw + wh + hl)) + MIN(lw, MIN(wh, hl));
		ribbon += (2 * MIN(dim[0] + dim[1],
		                   MIN(dim[1] + dim[2], dim[2] + dim[0]))) +
		          (lw * dim[2]);
	}
	if (printf("%lld\n%lld\n", paper, ribbon) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
