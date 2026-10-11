/* 2015 day 04, 00 naive: https://adventofcode.com/2015/day/4
 *
 * approach: append each positive decimal integer and compute RFC 1321 MD5;
 *           stop at the first digest whose first five hex digits are zero.
 * cost: time O(N * (k + log N)), memory O(k + log N), one input pass;
 *       N is the first matching integer and k is the key length.
 * trade: recompute the full padded hash for every candidate, in order.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static void md5(char *message, size_t len, uint32_t digest[4]);
static uint64_t part1(const char *in, size_t len);

static void
md5(char *message, size_t len, uint32_t digest[4])
{
	static const uint32_t constants[64] = {
	    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf,
	    0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af,
	    0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e,
	    0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6,
	    0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
	    0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122,
	    0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039,
	    0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97,
	    0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d,
	    0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
	};
	static const unsigned int shifts[16] = {
	    7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21,
	};
	uint32_t words[16], a, b, c, d, f, tmp;
	uint64_t bits = (uint64_t)len * 8;
	size_t total = len + 1, off, i, j, g;
	unsigned int s;

	while (total % 64 != 56)
		total++;
	memset(message + len, 0, total + 8 - len);
	message[len] = '\x80';
	for (i = 0; i < 8; i++)
		message[total + i] = (char)(unsigned char)(bits >> (i * 8));
	total += 8;
	digest[0] = 0x67452301;
	digest[1] = 0xefcdab89;
	digest[2] = 0x98badcfe;
	digest[3] = 0x10325476;
	for (off = 0; off < total; off += 64) {
		for (i = 0; i < 16; i++) {
			words[i] = 0;
			for (j = 0; j < 4; j++)
				words[i] |= (uint32_t)(unsigned char)
				                message[off + (i * 4) + j]
				            << (j * 8);
		}
		a = digest[0];
		b = digest[1];
		c = digest[2];
		d = digest[3];
		for (i = 0; i < 64; i++) {
			if (i < 16) {
				f = (b & c) | (~b & d);
				g = i;
			} else if (i < 32) {
				f = (b & d) | (c & ~d);
				g = ((5 * i) + 1) % 16;
			} else if (i < 48) {
				f = b ^ c ^ d;
				g = ((3 * i) + 5) % 16;
			} else {
				f = c ^ (b | ~d);
				g = (7 * i) % 16;
			}
			s = shifts[((i / 16) * 4) + (i % 4)];
			/* Explicit modulo sums and masked shifts satisfy
			 * UBSan. */
			f = (uint32_t)((uint64_t)a + f + constants[i] +
			               words[g]);
			f = ((f & (UINT32_MAX >> s)) << s) | (f >> (32 - s));
			tmp = d;
			d = c;
			c = b;
			b = (uint32_t)((uint64_t)b + f);
			a = tmp;
		}
		digest[0] = (uint32_t)((uint64_t)digest[0] + a);
		digest[1] = (uint32_t)((uint64_t)digest[1] + b);
		digest[2] = (uint32_t)((uint64_t)digest[2] + c);
		digest[3] = (uint32_t)((uint64_t)digest[3] + d);
	}
}

static uint64_t
part1(const char *in, size_t len)
{
	char *message;
	uint32_t digest[4];
	uint64_t n;
	int digits;

	if (len > SIZE_MAX - 92 || len > UINT64_MAX / 8 - 20)
		die("key too long");
	message = ecalloc(len + 92, 1);
	memcpy(message, in, len);
	for (n = 1;; n++) {
		digits = snprintf(message + len, 21, "%" PRIu64, n);
		if (digits < 0 || digits >= 21)
			die("snprintf failed");
		md5(message, len + (size_t)digits, digest);
		/* MD5 emits each word little-endian: bytes 0, 1, then a
		 * nibble. */
		if ((digest[0] & 0x00f0ffff) == 0)
			break;
		if (n == UINT64_MAX)
			die("candidate overflow");
	}
	free(message);
	return n;
}

int
main(void)
{
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	if (len && in[len - 1] == '\n')
		len--;
	if (len && in[len - 1] == '\r')
		len--;
	if (printf("%" PRIu64 "\n", part1(in, len)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
