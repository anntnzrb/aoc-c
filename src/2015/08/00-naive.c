/* 2015 day 08, 00 naive: https://adventofcode.com/2015/day/8
 *
 * approach: scan each quoted literal, counting source and decoded lengths;
 * each allowed escape contributes one decoded character.
 * cost: time O(n), memory O(n) for the input buffer, one pass
 * trade: count both lengths directly without materializing decoded strings
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t escape(const char *in, size_t len);
static size_t part1(const char *in, size_t len);
static size_t part2(const char *in, size_t len);

static size_t
escape(const char *in, size_t len)
{
	if (!len)
		die("incomplete escape");
	if (in[0] == 'x') {
		if (len < 3 || !isxdigit((unsigned char)in[1]) ||
		    !isxdigit((unsigned char)in[2]))
			die("invalid hexadecimal escape");
		return 3;
	}
	if (in[0] != '\\' && in[0] != '"')
		die("invalid escape");
	return 1;
}

static size_t
part1(const char *in, size_t len)
{
	size_t i, start, code, memory;

	code = memory = 0;
	for (i = 0; i < len;) {
		if (isspace((unsigned char)in[i])) {
			i++;
			continue;
		}
		start = i;
		if (in[i++] != '"')
			die("expected opening quote");
		while (i < len && in[i] != '"') {
			if (in[i++] == '\\')
				i += escape(in + i, len - i);
			memory++;
		}
		if (i == len)
			die("missing closing quote");
		code += ++i - start;
	}
	return code - memory;
}

int
main(void)
{
	char *in;
	size_t len, answer;

	in = slurp(stdin, &len);
	answer = part1(in, len);
	free(in);
	if (printf("%zu\n", answer) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
