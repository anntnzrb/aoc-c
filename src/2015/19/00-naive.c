/* 2015 day 19, 00 naive: https://adventofcode.com/2015/day/19
 *
 * approach: build every single replacement, sort, and count distinct strings.
 * cost: O(R L^2 + K L log K) time, O(K L) memory, one input pass;
 *       R rules, L molecule length, K matching replacement positions.
 * trade: copying whole molecules keeps replacement and deduplication direct.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	char *from, *to;
} Rule;

static int compare(const void *a, const void *b);
static size_t part1(const Rule *rules, size_t nrules, const char *molecule);

static int
compare(const void *a, const void *b)
{
	const char *const *x = a, *const *y = b;

	return strcmp(*x, *y);
}

static size_t
part1(const Rule *rules, size_t nrules, const char *molecule)
{
	char **made = NULL, *s;
	const char *p;
	size_t i, n = 0, distinct = 0, len = strlen(molecule), a, b, pos;

	for (i = 0; i < nrules; i++) {
		a = strlen(rules[i].from);
		b = strlen(rules[i].to);
		for (p = molecule; (p = strstr(p, rules[i].from)); p++) {
			pos = (size_t)(p - molecule);
			s = ecalloc(len - a + b + 1, 1);
			memcpy(s, molecule, pos);
			memcpy(s + pos, rules[i].to, b);
			memcpy(s + pos + b, p + a, len - pos - a + 1);
			made = ereallocarray(made, n + 1, sizeof(*made));
			made[n++] = s;
		}
	}
	if (n)
		qsort(made, n, sizeof(*made), compare);
	for (i = 0; i < n; i++)
		if (!i || strcmp(made[i], made[i - 1]))
			distinct++;
	for (i = 0; i < n; i++)
		free(made[i]);
	free(made);
	return distinct;
}

int
main(void)
{
	Rule *rules = NULL;
	char *in, *line, *next, *arrow, *molecule = NULL;
	size_t len, nrules = 0;

	in = slurp(stdin, &len);
	for (line = in; *line; line = next) {
		next = strchr(line, '\n');
		if (next)
			*next++ = 0;
		else
			next = line + strlen(line);
		if (!*line)
			continue;
		if ((arrow = strstr(line, " => "))) {
			*arrow = 0;
			rules =
			    ereallocarray(rules, nrules + 1, sizeof(*rules));
			rules[nrules].from = line;
			rules[nrules++].to = arrow + 4;
		} else {
			molecule = line;
		}
	}
	if (printf("%zu\n", molecule ? part1(rules, nrules, molecule) : 0) < 0)
		die("printf:");
	free(rules);
	free(in);
	return EXIT_SUCCESS;
}
