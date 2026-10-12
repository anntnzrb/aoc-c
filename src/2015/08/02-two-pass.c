/* 2015 day 08, 02 two-pass: https://adventofcode.com/2015/day/8
 *
 * approach: scan escape introducers and unescaped quotes for part 1,
 * then count every raw quote and backslash for the encoding expansion.
 * cost: time O(n), memory O(n) for the input buffer, two passes
 * trade: the independent second pass vectorizes, at the cost of buffering;
 * bench 8 50 (Linux) used 95236 work instructions vs stream-skip 60254.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	/* Each byte adds at most three to first and two to second. */
	if (len > SIZE_MAX / 3)
		die("length overflow");
	size_t quotes = 0, first = 0;

	for (size_t i = 0; i < len; i++) {
		if (in[i] == '"') {
			quotes++;
		} else if (in[i] == '\\') {
			if (++i == len)
				die("incomplete escape");
			first += in[i] == 'x' ? 3 : 1;
		}
	}
	first += quotes;
	size_t second = quotes;
	for (size_t i = 0; i < len; i++)
		second += (size_t)(in[i] == '"' || in[i] == '\\');
	free(in);
	if (printf("%zu\n%zu\n", first, second) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
