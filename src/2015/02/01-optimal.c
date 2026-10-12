/* 2015 day 02, 01 optimal: https://adventofcode.com/2015/day/2
 *
 * approach: stream decimal dimensions, retaining partial numbers across
 * reads. Two comparisons move the largest dimension to the third slot;
 * the other two give the smallest face and perimeter without sorting.
 * Reuse their product and sum: paper = 3ab + 2c(a+b), ribbon = 2(a+b)+abc.
 * Accept positive decimal dimensions with x separators and final newlines;
 * reject malformed input and dimensions or answers exceeding LLONG_MAX.
 * cost: time O(n), memory O(1) (64 KiB buffer), one pass over n bytes.
 * bound: every dimension can change both answers, requiring every byte;
 * O(n) time, O(1) memory and one pass attain the asymptotic lower bound.
 * Decimal validation and checked arithmetic limit the remaining work.
 * rejected: just bench 2 50, Linux, 1000 boxes, work instructions (median
 * and range of 5 full-minus-empty perf pairs): 00's slurp + strtoll,
 * 492,685 (492,685-492,685), O(n) memory. Streaming volume/max for the
 * smallest face: 261,705 (261,705-261,705). Three face products + minima:
 * 256,589 (256,586-256,593). The latter with a constant decimal-overflow
 * threshold: 240,235 (240,235-240,235). Moving the maximum and reusing
 * product/sum, with conditional separators: 223,857 (223,853-223,861).
 * Here, separator table: 214,857 (214,857-214,857), pinned to CPU 0 to
 * avoid noisy counters when migrating between this host's CPU types.
 */
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

static long long
decimal(long long v, int digit)
{
	if (ckd_mul(&v, v, 10) || ckd_add(&v, v, digit))
		die("dimension overflow");
	return v;
}

int
main(void)
{
	static char buf[65536];
	static constexpr char separators[] = "xx\n";
	long long dim[3], v = 0;
	long long paper = 0, ribbon = 0;
	size_t n, field = 0;
	bool digits = false;

	while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
		for (size_t i = 0; i < n; i++) {
			if (buf[i] >= '0' && buf[i] <= '9') {
				v = decimal(v, buf[i] - '0');
				digits = true;
				continue;
			}
			if (!digits || !v || buf[i] != separators[field])
				die("invalid box dimensions");
			dim[field++] = v;
			v = 0;
			digits = false;
			if (field != 3)
				continue;
			field = 0;
			if (dim[0] > dim[2]) {
				long long t = dim[0];
				dim[0] = dim[2];
				dim[2] = t;
			}
			if (dim[1] > dim[2]) {
				long long t = dim[1];
				dim[1] = dim[2];
				dim[2] = t;
			}
			long long face = mul(dim[0], dim[1]);
			long long sides = add(dim[0], dim[1]);
			paper = add(
			    paper,
			    add(mul(add(face, mul(dim[2], sides)), 2), face));
			ribbon =
			    add(ribbon, add(mul(sides, 2), mul(face, dim[2])));
		}
	}
	if (ferror(stdin))
		die("fread:");
	if (digits || field)
		die("unterminated box");
	if (printf("%lld\n%lld\n", paper, ribbon) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
