/* molecule.h: day 19 input parsing and exact minimum-cost grammar oracle. */
#ifndef MOLECULE_H
#define MOLECULE_H
#include <stdckdint.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static constexpr size_t INF = SIZE_MAX / 4;

typedef struct {
	char *from, *to;
} Rule;

typedef struct {
	size_t from, left, right, cost;
} Production;

typedef struct {
	Production *rules;
	size_t count, symbols;
} Grammar;

typedef struct {
	Rule *rules;
	size_t count;
	const char *molecule;
} Input;

static size_t
tokenize(const char *s, char names[][64], size_t *nnames, size_t *tokens,
         size_t capacity)
{
	size_t n = 0;

	while (*s) {
		char name[3] = {*s++, '\0', '\0'};
		if (name[0] >= 'A' && name[0] <= 'Z' && *s >= 'a' && *s <= 'z')
			name[1] = *s++;
		if (n == capacity)
			die("too many molecule tokens");
		tokens[n++] = nameid(names, nnames, 256, name);
	}
	return n;
}

static void
addrule(Grammar *g, size_t parent, const size_t *rhs, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		Production p = {parent, rhs[i],
		                count == 1 ? INF : rhs[count - 1],
		                i == 0 ? 1 : 0};
		if (i + 2 < count)
			p.right = g->symbols++;
		g->rules =
		    ereallocarray(g->rules, g->count + 1, sizeof(*g->rules));
		g->rules[g->count++] = p;
		parent = p.right;
		if (i + 2 >= count)
			break;
	}
}

static Grammar
makegrammar(const Rule *rules, size_t nrules, char names[][64], size_t *nnames)
{
	for (size_t r = 0; r < nrules; r++) {
		size_t lhs[64], rhs[64];
		if (tokenize(rules[r].from, names, nnames, lhs, LEN(lhs)) != 1)
			die("fabrication requires single-token rule sources");
		(void)tokenize(rules[r].to, names, nnames, rhs, LEN(rhs));
	}
	/* Token parsing is exact only when no rule can match inside a token.
	 */
	for (size_t r = 0; r < nrules; r++)
		for (size_t i = 0; i < *nnames; i++)
			if (strstr(names[i], rules[r].from) &&
			    strcmp(names[i], rules[r].from) != 0)
				die("replacement crosses molecule token "
				    "boundaries");
	Grammar g = {.symbols = *nnames};
	for (size_t r = 0; r < nrules; r++) {
		size_t lhs[64], rhs[64];
		(void)tokenize(rules[r].from, names, nnames, lhs, LEN(lhs));
		size_t count =
		    tokenize(rules[r].to, names, nnames, rhs, LEN(rhs));
		if (!count)
			die("empty replacement");
		addrule(&g, lhs[0], rhs, count);
	}
	return g;
}

static void
fillcell(const Grammar *g, size_t *chart, size_t side, size_t start,
         size_t span)
{
	size_t *cell = chart + (((start * side) + start + span) * g->symbols);
	for (size_t r = 0; r < g->count; r++) {
		Production p = g->rules[r];
		if (p.right == INF)
			continue;
		for (size_t split = start + 1; split < start + span; split++) {
			const size_t *left =
			    chart + (((start * side) + split) * g->symbols);
			const size_t *right =
			    chart +
			    (((split * side) + start + span) * g->symbols);
			size_t j = left[p.left], k = right[p.right];
			if (j != INF && k != INF)
				cell[p.from] =
				    MIN(cell[p.from], j + k + p.cost);
		}
	}
	/* Close unary rules, including the example's e => H and e => O. */
	bool changed;
	do {
		changed = false;
		for (size_t r = 0; r < g->count; r++) {
			Production p = g->rules[r];
			if (p.right == INF && cell[p.left] != INF &&
			    cell[p.left] + p.cost < cell[p.from]) {
				cell[p.from] = cell[p.left] + p.cost;
				changed = true;
			}
		}
	} while (changed);
}

static size_t
part2(const Rule *rules, size_t nrules, const char *molecule)
{
	bool source = false;

	for (size_t r = 0; r < nrules; r++)
		if (strcmp(rules[r].from, "e") == 0)
			source = true;
	if (!source)
		return 0;
	char names[256][64] = {};
	size_t nnames = 0;
	size_t electron = nameid(names, &nnames, LEN(names), "e");
	size_t *target = ecalloc(strlen(molecule) + 1, sizeof(*target));
	size_t n =
	    tokenize(molecule, names, &nnames, target, strlen(molecule) + 1);
	Grammar g = makegrammar(rules, nrules, names, &nnames);
	size_t side = n + 1, count;
	if (ckd_mul(&count, side, side) || ckd_mul(&count, count, g.symbols))
		die("molecule chart too large");
	size_t *chart = ecalloc(count, sizeof(*chart));
	for (size_t i = 0; i < count; i++)
		chart[i] = INF;
	for (size_t i = 0; i < n; i++)
		chart[(((i * side) + i + 1) * g.symbols) + target[i]] = 0;
	for (size_t span = 1; span <= n; span++)
		for (size_t start = 0; start + span <= n; start++)
			fillcell(&g, chart, side, start, span);
	size_t answer = chart[(n * g.symbols) + electron];
	free(chart);
	free(target);
	free(g.rules);
	if (answer == INF)
		die("medicine cannot be fabricated from e");
	return answer;
}

static Input
parse(char *in)
{
	Input data = {};
	char *next;

	for (char *line = in; *line; line = next) {
		next = strchr(line, '\n');
		if (next)
			*next++ = '\0';
		else
			next = line + strlen(line);
		if (!*line)
			continue;
		char *arrow = strstr(line, " => ");
		if (arrow) {
			*arrow = '\0';
			data.rules = ereallocarray(data.rules, data.count + 1,
			                           sizeof(*data.rules));
			data.rules[data.count].from = line;
			data.rules[data.count++].to = arrow + 4;
		} else {
			data.molecule = line;
		}
	}
	return data;
}

#endif
