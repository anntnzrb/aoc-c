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
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

int
main(void)
{
	char buf[8192];
	size_t n, i, first = 0, second = 0;
	int escaped = 0;

	while ((n = fread(buf, 1, sizeof(buf), stdin))) {
		i = 0;
		if (escaped) {
			first += buf[0] == 'x' ? 2 : 0;
			second += (size_t)(buf[0] == '"' || buf[0] == '\\');
			escaped = 0;
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
