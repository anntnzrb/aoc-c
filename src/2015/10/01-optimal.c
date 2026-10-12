/* 2015 day 10, 01 optimal: https://adventofcode.com/2015/day/10
 *
 * approach: reuse two buffers, emit counts directly, count the final output
 * cost: time O(sum of round lengths), memory O(max length), 50 passes
 * bound: each round grows at most 2x, so fixed 50 rounds take O(n) time,
 *        matching input reading; buffer space is not a proven lower bound
 * rejected: streaming counter 452677122 work (452677120-452677123);
 *           whole-run scan wins: 440375010 (440375008-440375013);
 *           00 formatting every count 8437318901 (8437318848-8437319170).
 *           Linux, just bench 10 20; work is startup-subtracted, n=5.
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t
putcount(char *out, size_t count)
{
	if (count < 10) {
		out[0] = (char)('0' + (int)count);
		return 1;
	}
	size_t digits = 1;
	for (size_t value = count; value >= 10; value /= 10)
		digits++;
	size_t i = digits;
	do {
		out[--i] = (char)('0' + (int)(count % 10));
		count /= 10;
	} while (i);
	return digits;
}

static size_t
transform(const char *in, size_t len, char *out)
{
	size_t i, j, n = 0;

	for (i = 0; i < len; i = j) {
		for (j = i + 1; j < len && in[j] == in[i]; j++)
			;
		n += putcount(out + n, j - i);
		out[n++] = in[i];
	}
	return n;
}

static size_t
finalsize(const char *in, size_t len)
{
	size_t maximum;
	if (ckd_mul(&maximum, len, 2))
		die("sequence too long");
	size_t n = 0;

	for (size_t i = 0, j; i < len; i = j) {
		for (j = i + 1; j < len && in[j] == in[i]; j++)
			;
		n += 2;
		for (size_t count = j - i; count >= 10; count /= 10)
			n++;
	}
	return n;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len), *out = NULL;
	size_t incap;
	if (ckd_add(&incap, len, 1))
		die("sequence too long");
	while (len && (in[len - 1] == '\n' || in[len - 1] == '\r'))
		len--;
	for (size_t i = 0; i < len; i++)
		if (in[i] < '0' || in[i] > '9')
			die("expected digits");
	size_t part1 = len, outcap = 0;
	for (int round = 0; len && round < 49; round++) {
		size_t needed;
		if (ckd_mul(&needed, len, 2))
			die("sequence too long");
		/* A run of k source digits emits at most 2k digits. */
		if (!out || outcap < needed) {
			out = ereallocarray(out, len, 2);
			outcap = needed;
		}
		len = transform(in, len, out);
		char *tmp = in;
		in = out;
		out = tmp;
		size_t tmpcap = incap;
		incap = outcap;
		outcap = tmpcap;
		if (round == 39)
			part1 = len;
	}
	if (printf("%zu\n%zu\n", part1, finalsize(in, len)) < 0)
		die("printf:");
	free(in);
	free(out);
	return EXIT_SUCCESS;
}
