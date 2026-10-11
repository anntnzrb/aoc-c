/* molecule.h: day 19 input parsing and exact minimum-cost grammar oracle. */
#ifndef MOLECULE_H
#define MOLECULE_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#define INF (SIZE_MAX / 4)

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

static size_t tokenize(const char *s, char names[][64], size_t *nnames,
                       size_t *tokens, size_t capacity);
static void addrule(Grammar *g, size_t parent, const size_t *rhs,
                    size_t count);
static Grammar makegrammar(const Rule *rules, size_t nrules, char names[][64],
                           size_t *nnames);
static void fillcell(const Grammar *g, size_t *chart, size_t side,
                     size_t start, size_t span);
static size_t part2(const Rule *rules, size_t nrules, const char *molecule);

static Input parse(char *in);

static size_t
tokenize(const char *s, char names[][64], size_t *nnames, size_t *tokens,
         size_t capacity)
{
	char name[3];
	size_t n = 0;

	while (*s) {
		name[0] = *s++;
		name[1] = '\0';
		name[2] = '\0';
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
	Production p;
	size_t i;

	for (i = 0; i < count; i++) {
		p.from = parent;
		p.left = rhs[i];
		p.right = count == 1 ? INF : rhs[count - 1];
		p.cost = i == 0 ? 1 : 0;
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
	Grammar g = {NULL, 0, 0};
	size_t lhs[64], rhs[64], r, i, count;

	for (r = 0; r < nrules; r++) {
		if (tokenize(rules[r].from, names, nnames, lhs, LEN(lhs)) != 1)
			die("fabrication requires single-token rule sources");
		(void)tokenize(rules[r].to, names, nnames, rhs, LEN(rhs));
	}
	/* Token parsing is exact only when no rule can match inside a token.
	 */
	for (r = 0; r < nrules; r++)
		for (i = 0; i < *nnames; i++)
			if (strstr(names[i], rules[r].from) &&
			    strcmp(names[i], rules[r].from) != 0)
				die("replacement crosses molecule token "
				    "boundaries");
	g.symbols = *nnames;
	for (r = 0; r < nrules; r++) {
		(void)tokenize(rules[r].from, names, nnames, lhs, LEN(lhs));
		count = tokenize(rules[r].to, names, nnames, rhs, LEN(rhs));
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
	Production p;
	const size_t *left, *right;
	size_t *cell, r, split, j, k;
	int changed;

	cell = chart + (((start * side) + start + span) * g->symbols);
	for (r = 0; r < g->count; r++) {
		p = g->rules[r];
		if (p.right == INF)
			continue;
		for (split = start + 1; split < start + span; split++) {
			left = chart + (((start * side) + split) * g->symbols);
			right = chart +
			        (((split * side) + start + span) * g->symbols);
			j = left[p.left];
			k = right[p.right];
			if (j != INF && k != INF)
				cell[p.from] =
				    MIN(cell[p.from], j + k + p.cost);
		}
	}
	/* Close unary rules, including the example's e => H and e => O. */
	do {
		changed = 0;
		for (r = 0; r < g->count; r++) {
			p = g->rules[r];
			if (p.right == INF && cell[p.left] != INF &&
			    cell[p.left] + p.cost < cell[p.from]) {
				cell[p.from] = cell[p.left] + p.cost;
				changed = 1;
			}
		}
	} while (changed);
}

static size_t
part2(const Rule *rules, size_t nrules, const char *molecule)
{
	char names[256][64] = {{0}};
	Grammar g;
	size_t nnames = 0, n, side, count, i, r, span, start, answer, electron;
	size_t *target, *chart;
	int source = 0;

	for (r = 0; r < nrules; r++)
		if (strcmp(rules[r].from, "e") == 0)
			source = 1;
	if (!source)
		return 0;
	electron = nameid(names, &nnames, LEN(names), "e");
	target = ecalloc(strlen(molecule) + 1, sizeof(*target));
	n = tokenize(molecule, names, &nnames, target, strlen(molecule) + 1);
	g = makegrammar(rules, nrules, names, &nnames);
	side = n + 1;
	count = side * side * g.symbols;
	chart = ecalloc(count, sizeof(*chart));
	for (i = 0; i < count; i++)
		chart[i] = INF;
	for (i = 0; i < n; i++)
		chart[(((i * side) + i + 1) * g.symbols) + target[i]] = 0;
	for (span = 1; span <= n; span++)
		for (start = 0; start + span <= n; start++)
			fillcell(&g, chart, side, start, span);
	answer = chart[(n * g.symbols) + electron];
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
	Input data = {NULL, 0, NULL};
	char *line, *next, *arrow;

	for (line = in; *line; line = next) {
		next = strchr(line, '\n');
		if (next)
			*next++ = '\0';
		else
			next = line + strlen(line);
		if (!*line)
			continue;
		if ((arrow = strstr(line, " => "))) {
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
