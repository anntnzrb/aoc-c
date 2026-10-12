/* 2015 day 10, 00 naive: https://adventofcode.com/2015/day/10
 *
 * approach: replace each run with its count and digit, fifty times
 * cost: time O(sum of round lengths), memory O(max round length), 50 passes
 * trade: materialize every intermediate sequence for a direct simulation
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static char *
step(const char *in, size_t len, size_t *outlen)
{
	/* A run of k digits emits at most 2k digits, even for large counts. */
	size_t capacity;
	if (ckd_mul(&capacity, len, 2))
		die("sequence too long");
	char *out = ecalloc(capacity, 1);
	size_t n = 0;
	for (size_t i = 0, j; i < len; i = j) {
		for (j = i + 1; j < len && in[j] == in[i]; j++)
			;
		char count[(3 * sizeof(size_t)) + 1];
		int digits = snprintf(count, sizeof(count), "%zu", j - i);
		if (digits < 0 || (size_t)digits >= sizeof(count))
			die("run count formatting failed");
		memcpy(out + n, count, (size_t)digits);
		n += (size_t)digits;
		out[n++] = in[i];
	}
	*outlen = n;
	return out;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	while (len && (in[len - 1] == '\n' || in[len - 1] == '\r'))
		len--;
	for (size_t i = 0; i < len; i++)
		if (in[i] < '0' || in[i] > '9')
			die("expected digits");
	size_t part1 = len;
	for (int round = 0; len && round < 50; round++) {
		char *next = step(in, len, &len);
		free(in);
		in = next;
		if (round == 39)
			part1 = len;
	}
	if (printf("%zu\n%zu\n", part1, len) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
