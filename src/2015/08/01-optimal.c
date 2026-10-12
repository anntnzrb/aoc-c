/* 2015 day 08, 01 optimal: https://adventofcode.com/2015/day/8
 *
 * approach: count only quotes and escape introducers; consume an escape
 * partner immediately, carrying it to the next buffer only when necessary.
 * cost: time O(n), memory O(1), one pass with an 8192-byte buffer
 * bound: any source byte can introduce an escape, requiring Omega(n) reads;
 * a fixed buffer and one boundary bit suffice, attaining O(1) space.
 * rejected: bench 8 50 (Linux), work instructions: two-pass 95236;
 * per-byte streaming state 142619; 00 naive 178040. This scan used 60254.
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static void
checklengths(size_t first, size_t second, size_t n)
{
	size_t limit;
	if (ckd_add(&limit, first, 4 * n) || ckd_add(&limit, second, 4 * n))
		die("length overflow");
}

int
main(void)
{
	char buf[8192];
	size_t n, first = 0, second = 0;
	bool escaped = false;

	while ((n = fread(buf, 1, sizeof(buf), stdin))) {
		checklengths(first, second, n);
		size_t i = 0;
		if (escaped) {
			first += buf[0] == 'x' ? 2 : 0;
			second += (size_t)(buf[0] == '"' || buf[0] == '\\');
			escaped = false;
			i++;
		}
		for (; i < n; i++) {
			if (buf[i] == '"') {
				first++;
				second += 2;
			} else if (buf[i] == '\\') {
				first++;
				second++;
				if (++i == n) {
					escaped = true;
					break;
				}
				first += buf[i] == 'x' ? 2 : 0;
				second +=
				    (size_t)(buf[i] == '"' || buf[i] == '\\');
			}
		}
	}
	if (ferror(stdin))
		die("fread:");
	if (printf("%zu\n%zu\n", first, second) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
