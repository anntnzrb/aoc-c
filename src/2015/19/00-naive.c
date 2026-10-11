/* 2015 day 19, 00 naive: https://adventofcode.com/2015/day/19
 *
 * approach: sort single replacements; find a minimum-cost grammar derivation
 *           of the target with interval dynamic programming from e.
 * cost: part 1 O(R L^2 + K L log K) time, O(K L) memory; part 2
 *       O(G N^3 + (G + U) S N^2) time, O(S N^2) memory, one input pass;
 *       R rules, L bytes, K matches, N tokens, S grammar symbols,
 *       G binary productions and U unary productions.
 * trade: exhaustive interval parsing proves the minimum without greedy
 *        reductions; binary helper productions carry zero replacement cost.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"
#include "molecule.h"

typedef struct {
	char *text;
} Molecule;

static int compare(const void *a, const void *b);
static size_t part1(const Rule *rules, size_t nrules, const char *molecule);

static int
compare(const void *a, const void *b)
{
	const Molecule *x = a, *y = b;

	return strcmp(x->text, y->text);
}

static size_t
part1(const Rule *rules, size_t nrules, const char *molecule)
{
	Molecule *made = NULL;
	char *s;
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
			made[n++].text = s;
		}
	}
	if (n)
		qsort(made, n, sizeof(*made), compare);
	for (i = 0; i < n; i++)
		if (!i || strcmp(made[i].text, made[i - 1].text) != 0)
			distinct++;
	for (i = 0; i < n; i++)
		free(made[i].text);
	free(made);
	return distinct;
}

int
main(void)
{
	Input data;
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	data = parse(in);
	if (printf("%zu\n%zu\n",
	           data.molecule ? part1(data.rules, data.count, data.molecule)
	                         : 0,
	           data.molecule ? part2(data.rules, data.count, data.molecule)
	                         : 0) < 0)
		die("printf:");
	free(data.rules);
	free(in);
	return EXIT_SUCCESS;
}
