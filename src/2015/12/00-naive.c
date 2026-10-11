/* 2015 day 12, 00 naive: https://adventofcode.com/2015/day/12
 *
 * approach: scan integers for part 1; sum JSON containers with a stack for
 * part 2, discarding an object subtotal when a direct property value is "red".
 * cost: time O(n), memory O(n) input and stack, two passes.
 * trade: keeps the direct part-1 scan and parses structure only for part 2.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static long long number(char **p);
static long long part1(char *in);
static void space(char **p);
static int string(char **p);
typedef struct {
	long long sum;
	int object, red;
} Frame;

static long long part2(char *in, size_t len);

static long long
number(char **p)
{
	char *end;
	long long n;

	errno = 0;
	n = strtoll(*p, &end, 10);
	if (errno || end == *p)
		die("invalid integer");
	*p = end;
	return n;
}

static long long
part1(char *in)
{
	char *p;
	long long sum = 0;

	for (p = in; *p;) {
		if (*p == '-' || (*p >= '0' && *p <= '9'))
			sum += number(&p);
		else
			p++;
	}
	return sum;
}

static void
space(char **p)
{
	while (isspace((unsigned char)**p))
		(*p)++;
}

static int
string(char **p)
{
	const char *start;
	int red;

	if (**p != '"')
		die("expected string");
	start = ++*p;
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
	red = *p - start == 3 && memcmp(start, "red", 3) == 0;
	(*p)++;
	return red;
}

static long long
part2(char *in, size_t len)
{
	Frame *stack;
	char *p;
	size_t depth = 0;
	long long sum;
	int red;

	stack = ecalloc(len + 1, sizeof(*stack));
	for (p = in; *p;) {
		if (*p == '{' || *p == '[') {
			depth++;
			stack[depth].sum = 0;
			stack[depth].red = 0;
			stack[depth].object = *p == '{';
			p++;
		} else if (*p == '}' || *p == ']') {
			if (!depth || stack[depth].object != (*p == '}'))
				die("unmatched container");
			sum = stack[depth].red ? 0 : stack[depth].sum;
			depth--;
			stack[depth].sum += sum;
			p++;
		} else if (*p == '"') {
			red = string(&p);
			space(&p);
			if (red && stack[depth].object && *p != ':')
				stack[depth].red = 1;
		} else if (*p == '-' || (*p >= '0' && *p <= '9')) {
			stack[depth].sum += number(&p);
		} else {
			p++;
		}
	}
	if (depth)
		die("unterminated container");
	sum = stack[0].sum;
	free(stack);
	return sum;
}

int
main(void)
{
	char *in;
	long long sum1, sum2;
	size_t len;

	in = slurp(stdin, &len);
	sum1 = part1(in);
	sum2 = part2(in, len);
	if (printf("%lld\n%lld\n", sum1, sum2) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
