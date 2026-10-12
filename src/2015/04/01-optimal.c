/* 2015 day 04, 01 optimal: https://adventofcode.com/2015/day/4
 *
 * approach: search both prefixes together; cache whole key blocks, increment
 *           packed decimal digits, and batch 16 unrolled MD5 hashes for SIMD.
 * cost: time O(k + N), memory O(k), one input pass and one fused search.
 * bound: black-box hashing needs Omega(k + N): read the key and reject every
 *        smaller candidate; cached blocks and amortized carries meet it.
 * rejected: just bench 4 5 on Linux, paired user-instruction work (n=5):
 *           scalar 6870898235 (6870898217-6870898566);
 *           batch4 7674111200 (7674111183-7674111237);
 *           batch8 9022531513 (9022531508-9022531518);
 *           monolithic batch16 3226872710 (3226872706-3226872715),
 *           which failed the cognitive-complexity gate.
 *           four-round batch16 wins: 3222796878 (3222796875-3222796878).
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static constexpr size_t LANES = 16;

#define MD5_F(b, c, d) ((x->b[lane] & x->c[lane]) | (~x->b[lane] & x->d[lane]))
#define MD5_G(b, c, d) ((x->b[lane] & x->d[lane]) | (x->c[lane] & ~x->d[lane]))
#define MD5_H(b, c, d) (x->b[lane] ^ x->c[lane] ^ x->d[lane])
#define MD5_I(b, c, d) (x->c[lane] ^ (x->b[lane] | ~x->d[lane]))
#define STEP(a, b, c, d, fn, k, s, t)                                         \
	for (size_t lane = 0; lane < LANES; lane++) {                         \
		x->a[lane] = (uint32_t)((uint64_t)x->a[lane] + fn(b, c, d) +  \
		                        block->words[k][lane] + (t));         \
		x->a[lane] = ((x->a[lane] & (UINT32_MAX >> (s))) << (s)) |    \
		             (x->a[lane] >> (32 - (s)));                      \
		x->a[lane] = (uint32_t)((uint64_t)x->a[lane] + x->b[lane]);   \
	}

typedef struct {
	uint32_t words[16][LANES];
} Block;

typedef struct {
	uint32_t a[LANES], b[LANES], c[LANES], d[LANES];
} State;

static void
putbyte(Block *blocks, size_t pos, size_t lane, unsigned char value)
{
	unsigned int shift = (unsigned int)(pos % 4) * 8;
	uint32_t *word = &blocks[pos / 64].words[(pos % 64) / 4][lane];
	*word =
	    (*word & ~(UINT32_C(0xff) << shift)) | ((uint32_t)value << shift);
}

static void
round1(State *x, const Block *block)
{
	STEP(a, b, c, d, MD5_F, 0, 7, 0xd76aa478)
	STEP(d, a, b, c, MD5_F, 1, 12, 0xe8c7b756)
	STEP(c, d, a, b, MD5_F, 2, 17, 0x242070db)
	STEP(b, c, d, a, MD5_F, 3, 22, 0xc1bdceee)
	STEP(a, b, c, d, MD5_F, 4, 7, 0xf57c0faf)
	STEP(d, a, b, c, MD5_F, 5, 12, 0x4787c62a)
	STEP(c, d, a, b, MD5_F, 6, 17, 0xa8304613)
	STEP(b, c, d, a, MD5_F, 7, 22, 0xfd469501)
	STEP(a, b, c, d, MD5_F, 8, 7, 0x698098d8)
	STEP(d, a, b, c, MD5_F, 9, 12, 0x8b44f7af)
	STEP(c, d, a, b, MD5_F, 10, 17, 0xffff5bb1)
	STEP(b, c, d, a, MD5_F, 11, 22, 0x895cd7be)
	STEP(a, b, c, d, MD5_F, 12, 7, 0x6b901122)
	STEP(d, a, b, c, MD5_F, 13, 12, 0xfd987193)
	STEP(c, d, a, b, MD5_F, 14, 17, 0xa679438e)
	STEP(b, c, d, a, MD5_F, 15, 22, 0x49b40821)
}

static void
round2(State *x, const Block *block)
{
	STEP(a, b, c, d, MD5_G, 1, 5, 0xf61e2562)
	STEP(d, a, b, c, MD5_G, 6, 9, 0xc040b340)
	STEP(c, d, a, b, MD5_G, 11, 14, 0x265e5a51)
	STEP(b, c, d, a, MD5_G, 0, 20, 0xe9b6c7aa)
	STEP(a, b, c, d, MD5_G, 5, 5, 0xd62f105d)
	STEP(d, a, b, c, MD5_G, 10, 9, 0x02441453)
	STEP(c, d, a, b, MD5_G, 15, 14, 0xd8a1e681)
	STEP(b, c, d, a, MD5_G, 4, 20, 0xe7d3fbc8)
	STEP(a, b, c, d, MD5_G, 9, 5, 0x21e1cde6)
	STEP(d, a, b, c, MD5_G, 14, 9, 0xc33707d6)
	STEP(c, d, a, b, MD5_G, 3, 14, 0xf4d50d87)
	STEP(b, c, d, a, MD5_G, 8, 20, 0x455a14ed)
	STEP(a, b, c, d, MD5_G, 13, 5, 0xa9e3e905)
	STEP(d, a, b, c, MD5_G, 2, 9, 0xfcefa3f8)
	STEP(c, d, a, b, MD5_G, 7, 14, 0x676f02d9)
	STEP(b, c, d, a, MD5_G, 12, 20, 0x8d2a4c8a)
}

static void
round3(State *x, const Block *block)
{
	STEP(a, b, c, d, MD5_H, 5, 4, 0xfffa3942)
	STEP(d, a, b, c, MD5_H, 8, 11, 0x8771f681)
	STEP(c, d, a, b, MD5_H, 11, 16, 0x6d9d6122)
	STEP(b, c, d, a, MD5_H, 14, 23, 0xfde5380c)
	STEP(a, b, c, d, MD5_H, 1, 4, 0xa4beea44)
	STEP(d, a, b, c, MD5_H, 4, 11, 0x4bdecfa9)
	STEP(c, d, a, b, MD5_H, 7, 16, 0xf6bb4b60)
	STEP(b, c, d, a, MD5_H, 10, 23, 0xbebfbc70)
	STEP(a, b, c, d, MD5_H, 13, 4, 0x289b7ec6)
	STEP(d, a, b, c, MD5_H, 0, 11, 0xeaa127fa)
	STEP(c, d, a, b, MD5_H, 3, 16, 0xd4ef3085)
	STEP(b, c, d, a, MD5_H, 6, 23, 0x04881d05)
	STEP(a, b, c, d, MD5_H, 9, 4, 0xd9d4d039)
	STEP(d, a, b, c, MD5_H, 12, 11, 0xe6db99e5)
	STEP(c, d, a, b, MD5_H, 15, 16, 0x1fa27cf8)
	STEP(b, c, d, a, MD5_H, 2, 23, 0xc4ac5665)
}

static void
round4(State *state, State *x, const Block *block, bool final)
{
	STEP(a, b, c, d, MD5_I, 0, 6, 0xf4292244)
	STEP(d, a, b, c, MD5_I, 7, 10, 0x432aff97)
	STEP(c, d, a, b, MD5_I, 14, 15, 0xab9423a7)
	STEP(b, c, d, a, MD5_I, 5, 21, 0xfc93a039)
	STEP(a, b, c, d, MD5_I, 12, 6, 0x655b59c3)
	STEP(d, a, b, c, MD5_I, 3, 10, 0x8f0ccc92)
	STEP(c, d, a, b, MD5_I, 10, 15, 0xffeff47d)
	STEP(b, c, d, a, MD5_I, 1, 21, 0x85845dd1)
	STEP(a, b, c, d, MD5_I, 8, 6, 0x6fa87e4f)
	STEP(d, a, b, c, MD5_I, 15, 10, 0xfe2ce6e0)
	STEP(c, d, a, b, MD5_I, 6, 15, 0xa3014314)
	STEP(b, c, d, a, MD5_I, 13, 21, 0x4e0811a1)
	STEP(a, b, c, d, MD5_I, 4, 6, 0xf7537e82)
	/* Steps 62-64 only update d, c, b; the leading digest bytes need a. */
	for (size_t lane = 0; lane < LANES; lane++)
		state->a[lane] =
		    (uint32_t)((uint64_t)state->a[lane] + x->a[lane]);
	if (final)
		return;
	STEP(d, a, b, c, MD5_I, 11, 10, 0xbd3af235)
	STEP(c, d, a, b, MD5_I, 2, 15, 0x2ad7d2bb)
	STEP(b, c, d, a, MD5_I, 9, 21, 0xeb86d391)
	for (size_t lane = 0; lane < LANES; lane++) {
		state->b[lane] =
		    (uint32_t)((uint64_t)state->b[lane] + x->b[lane]);
		state->c[lane] =
		    (uint32_t)((uint64_t)state->c[lane] + x->c[lane]);
		state->d[lane] =
		    (uint32_t)((uint64_t)state->d[lane] + x->d[lane]);
	}
}

static void
compress(State *state, const Block *block, bool final)
{
	State x = *state;

	round1(&x, block);
	round2(&x, block);
	round3(&x, block);
	round4(state, &x, block, final);
}

static void
advance(Block *blocks, size_t end, size_t lane)
{
	unsigned int carry = (unsigned int)LANES;

	while (carry) {
		end--;
		uint32_t word = blocks[end / 64].words[(end % 64) / 4][lane];
		unsigned int digit =
		    ((word >> ((end % 4) * 8)) & 0xff) - '0' + carry;
		carry = digit / 10;
		putbyte(blocks, end, lane,
		        (unsigned char)('0' + (digit % 10)));
	}
}

static State
keyhash(const char *in, size_t len)
{
	State state = {};

	for (size_t lane = 0; lane < LANES; lane++) {
		state.a[lane] = 0x67452301;
		state.b[lane] = 0xefcdab89;
		state.c[lane] = 0x98badcfe;
		state.d[lane] = 0x10325476;
	}
	/* Whole key blocks never change between candidates. */
	Block block = {};
	for (size_t off = 0; len - off >= 64; off += 64) {
		for (size_t i = 0; i < 64; i++)
			for (size_t lane = 0; lane < LANES; lane++)
				putbyte(&block, i, lane,
				        (unsigned char)in[off + i]);
		compress(&state, &block, false);
	}
	return state;
}

static void
prepare(Block *blocks, const char *in, size_t len, size_t digits,
        uint64_t base, uint64_t limit)
{
	uint64_t bits = (uint64_t)(len + digits) * 8;
	size_t rem = len % 64, last = rem + digits >= 56 ? 1 : 0;
	size_t count = (size_t)MIN((uint64_t)LANES, limit - base + 1);

	memset(blocks, 0, 2 * sizeof(*blocks));
	for (size_t lane = 0; lane < LANES; lane++) {
		for (size_t i = 0; i < rem; i++)
			putbyte(blocks, i, lane,
			        (unsigned char)in[len - rem + i]);
		char decimal[21];
		int written = snprintf(decimal, sizeof(decimal), "%" PRIu64,
		                       base + MIN(lane, count - 1));
		if (written < 0 || (size_t)written != digits)
			die("snprintf failed");
		for (size_t i = 0; i < digits; i++)
			putbyte(blocks, rem + i, lane,
			        (unsigned char)decimal[i]);
		putbyte(blocks, rem + digits, lane, 0x80);
		for (size_t i = 0; i < 8; i++)
			putbyte(blocks, (last * 64) + 56 + i, lane,
			        (unsigned char)(bits >> (i * 8)));
	}
}

static bool
matches(const State *state, uint64_t base, size_t count, uint64_t answers[2])
{
	/* Lane order preserves the lowest matching candidate. */
	for (size_t lane = 0; lane < count; lane++) {
		if (!answers[0] && (state->a[lane] & 0x00f0ffff) == 0)
			answers[0] = base + lane;
		if ((state->a[lane] & 0x00ffffff) == 0) {
			answers[1] = base + lane;
			return true;
		}
	}
	return false;
}

static bool
searchwidth(const State *prefix, Block *blocks, size_t end, uint64_t base,
            uint64_t limit, uint64_t answers[2])
{
	for (;;) {
		size_t count = (size_t)MIN((uint64_t)LANES, limit - base + 1);
		State state = *prefix;
		compress(&state, blocks, end < 56);
		if (end >= 56)
			compress(&state, blocks + 1, true);
		if (matches(&state, base, count, answers))
			return true;
		if (limit - base < LANES)
			return false;
		base += LANES;
		count = (size_t)MIN((uint64_t)LANES, limit - base + 1);
		for (size_t lane = 0; lane < count; lane++)
			advance(blocks, end, lane);
	}
}

static void
mine(const char *in, size_t len, uint64_t answers[2])
{
	if (len > UINT64_MAX / 8 - 20)
		die("key too long");
	State prefix = keyhash(in, len);
	uint64_t base = 1, limit = 9;
	/* Within a decimal width, only the carry digits change. */
	for (size_t digits = 1; digits <= 20; digits++) {
		Block blocks[2];
		prepare(blocks, in, len, digits, base, limit);
		if (searchwidth(&prefix, blocks, (len % 64) + digits, base,
		                limit, answers))
			return;
		if (limit == UINT64_MAX)
			die("candidate overflow");
		base = limit + 1;
		limit = limit > (UINT64_MAX - 9) / 10 ? UINT64_MAX
		                                      : (limit * 10) + 9;
	}
	die("candidate overflow");
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	if (len && in[len - 1] == '\n')
		len--;
	if (len && in[len - 1] == '\r')
		len--;
	if (len) {
		uint64_t answers[2] = {};
		mine(in, len, answers);
		if (printf("%" PRIu64 "\n%" PRIu64 "\n", answers[0],
		           answers[1]) < 0)
			die("printf:");
	}
	free(in);
	return EXIT_SUCCESS;
}
