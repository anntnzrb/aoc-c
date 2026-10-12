/* 2015 day 12, 01 optimal: https://adventofcode.com/2015/day/12
 *
 * approach: fuse both sums in one token scan; restore a running sum checkpoint
 * at object close when a direct property value is "red".
 * cost: time O(n), memory O(n) input plus O(depth) stack, one pass.
 * bound: every byte can change a number or object exclusion, requiring
 * Omega(n) inspection; one scan attains this time and pass bound.
 * rejected: just bench 12 50 on Linux: fused subtotals work 543671
 * (543671-543672); 00 two-pass strtoll work 1726303 (1726302-1726304).
 * Rollback checkpoints won with work 537095 (537093-537095), n=5 pairs.
 * Inline red classification used 530602 work but failed the complexity gate.
 */
#include <ctype.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	long long sum;
	bool object, red;
} Frame;

static long long
add(long long sum, long long value)
{
	if (ckd_add(&sum, sum, value))
		die("sum overflow");
	return sum;
}

static long long
number(char **p)
{
	long long prefix = 0;
	bool negative = **p == '-';
	if (negative)
		(*p)++;
	if (**p < '0' || **p > '9')
		die("invalid integer");
	/* Three decimal digits fit; check only the unbounded tail. */
	for (unsigned digits = 0; digits < 3; digits++) {
		prefix = (prefix * 10) + (**p - '0');
		(*p)++;
		if (**p < '0' || **p > '9')
			return negative ? -prefix : prefix;
	}
	long long n = negative ? -prefix : prefix;
	do {
		int digit = **p - '0';
		if (ckd_mul(&n, n, 10) ||
		    ckd_add(&n, n, negative ? -digit : digit))
			die("integer overflow");
		(*p)++;
	} while (**p >= '0' && **p <= '9');
	return n;
}

static bool
redvalue(char **p)
{
	const char *start = ++*p;
	while (**p && **p != '"') {
		if (**p == '\\') {
			(*p)++;
			if (!**p)
				die("unterminated escape");
		}
		(*p)++;
	}
	if (!**p)
		die("unterminated string");
	bool red = *p - start == 3 && memcmp(start, "red", 3) == 0;
	(*p)++;
	if (!red)
		return false;
	while (isspace((unsigned char)**p))
		(*p)++;
	return **p != ':';
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	size_t depth = 0, cap = 64;
	Frame *stack = ecalloc(cap, sizeof(*stack));
	long long sum1 = 0, sum2 = 0;
	for (char *p = in; *p;) {
		switch (*p) {
		case '{':
		case '[':
			if (++depth == cap) {
				cap *= 2;
				stack =
				    ereallocarray(stack, cap, sizeof(*stack));
			}
			stack[depth].sum = sum2;
			stack[depth].red = false;
			stack[depth].object = *p++ == '{';
			break;
		case '}':
		case ']':
			if (!depth || stack[depth].object != (*p == '}'))
				die("unmatched container");
			if (stack[depth].red)
				sum2 = stack[depth].sum;
			depth--;
			p++;
			break;
		case '"':
			stack[depth].red |=
			    redvalue(&p) && stack[depth].object;
			break;
		default:
			if (*p == '-' || (*p >= '0' && *p <= '9')) {
				long long n = number(&p);
				sum1 = add(sum1, n);
				sum2 = add(sum2, n);
			} else {
				p++;
			}
			break;
		}
	}
	if (depth)
		die("unterminated container");
	if (printf("%lld\n%lld\n", sum1, sum2) < 0)
		die("printf:");
	free(stack);
	free(in);
	return EXIT_SUCCESS;
}
