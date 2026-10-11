/* 2015 day 19, 01 optimal: https://adventofcode.com/2015/day/19
 *
 * approach: trim each product against the original into a canonical edit;
 *           hash exact edit keys. A checked rule-weight invariant counts
 *           fabrication steps; other grammars use the exact chart oracle.
 * cost: expected O(R N + K M^2) time, O(N + K M) memory for calibration;
 *       invariant fabrication O(B) time, O(N) extra memory for fixed grammar;
 *       O(R) molecule passes. N bytes, R rules, K matches, M longest rule,
 *       B input bytes. Non-invariant fabrication uses the chart's cost.
 * bound: reading the molecule requires Omega(N); canonicalization and hashing
 *        attain expected Theta(N) for a fixed grammar. Every checked rule
 *        raises W=tokens-Rn-Ar-2Y by one. The electron weight is inferred
 *        from its rules; the puzzle promises a fabricatable target, so every
 *        derivation has exactly W(target)-W(e) steps, attaining that bound.
 *        Exact edit keys use maximal common prefix then nonoverlapping
 *        common suffix, so identical products have identical keys.
 * rejected: sorted canonical keys: work 1133058 (1133057-1133059, n=5),
 *           wall med 902.1 us, cpu med 796.0 us, rss 5648 KiB;
 *           50 timing runs, identical compiler flags and input.
 *           Extra suffix table: work 985314 (985314-985314, n=5);
 *           unnecessary because the common-prefix cap bounds suffix scans.
 *           Kept hash: work 967752 (967751-967753, n=5), wall med 676.5 us,
 *           cpu med 587.0 us, rss 5652 KiB; 50 timing runs on Linux x86_64.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"
#include "molecule.h"

typedef struct {
	size_t position, removed;
	char *text;
} Edit;

static char productchar(const Rule *rule, const char *s, size_t pos,
                        size_t index);
static void shifts(const Rule *rule, const char *s, size_t n, size_t *next);
static Edit normalize(const Rule *rule, const char *s, size_t n, size_t pos,
                      size_t *next, int *ready);
static int compare(const void *a, const void *b);
static uint64_t edithash(const Edit *edit);
static size_t hashcount(const Edit *edits, size_t count);
static size_t calibrate(const Rule *rules, size_t count, const char *s);
static long long weight(const char *s);
static int invariant(const Rule *rules, size_t count, long long *electron);
static size_t fabricate(const Rule *rules, size_t count, const char *s);

static char
productchar(const Rule *rule, const char *s, size_t pos, size_t index)
{
	size_t a = strlen(rule->from), b = strlen(rule->to);

	if (index < pos)
		return s[index];
	if (index - pos < b)
		return rule->to[index - pos];
	return s[index - b + a];
}

static void
shifts(const Rule *rule, const char *s, size_t n, size_t *next)
{
	size_t a = strlen(rule->from), b = strlen(rule->to), limit, q, t;

	limit = MIN(n, n - a + b);
	next[limit] = limit;
	for (q = limit; q > 0;) {
		q--;
		next[q] = next[q + 1];
		if (b >= a) {
			if (q < b - a)
				continue;
			t = q - (b - a);
		} else {
			t = q + (a - b);
		}
		if (s[q] != s[t])
			next[q] = q;
	}
}

static Edit
normalize(const Rule *rule, const char *s, size_t n, size_t pos, size_t *next,
          int *ready)
{
	Edit edit;
	size_t a = strlen(rule->from), b = strlen(rule->to), out = n - a + b;
	size_t limit = MIN(n, out), first = pos, suffix, bytes, i;

	while (first < limit && first - pos < b &&
	       s[first] == rule->to[first - pos])
		first++;
	if (first < limit && first - pos == b) {
		if (!*ready) {
			shifts(rule, s, n, next);
			*ready = 1;
		}
		first = next[first];
	}
	suffix = MIN(n - pos - a, limit - first);
	/* The prefix cap prevents this scan from crossing before the edit. */
	while (suffix < limit - first &&
	       productchar(rule, s, pos, out - suffix - 1) ==
	           s[n - suffix - 1])
		suffix++;
	edit.position = first;
	edit.removed = n - suffix - first;
	bytes = out - first - suffix;
	edit.text = ecalloc(bytes + 1, 1);
	for (i = 0; i < bytes; i++)
		edit.text[i] = productchar(rule, s, pos, first + i);
	return edit;
}

static int
compare(const void *a, const void *b)
{
	const Edit *x = a, *y = b;

	if (x->position != y->position)
		return (x->position > y->position) -
		       (x->position < y->position);
	if (x->removed != y->removed)
		return (x->removed > y->removed) - (x->removed < y->removed);
	return strcmp(x->text, y->text);
}

static uint64_t
edithash(const Edit *edit)
{
	const unsigned char *p;
	uint64_t h = (uint64_t)edit->position,
	         removed = (uint64_t)edit->removed;

	h ^= ((removed & (UINT64_MAX >> 17)) << 17) | (removed >> 47);
	for (p = (const unsigned char *)edit->text; *p; p++)
		h = (((h & (UINT64_MAX >> 5)) << 5) | (h >> 59)) ^ *p;
	return h ^ (h >> 32) ^ (h >> 16);
}

static size_t
hashcount(const Edit *edits, size_t count)
{
	size_t *table, capacity = 1, i, slot, distinct = 0;

	if (!count)
		return 0;
	while (capacity <= count) {
		if (capacity > SIZE_MAX / 2)
			die("too many calibration keys");
		capacity *= 2;
	}
	if (capacity > SIZE_MAX / 2)
		die("too many calibration keys");
	table = ecalloc(capacity, 2 * sizeof(*table));
	capacity *= 2;
	for (i = 0; i < count; i++) {
		slot = (size_t)edithash(&edits[i]) & (capacity - 1);
		while (table[slot] &&
		       compare(&edits[i], &edits[table[slot] - 1]) != 0)
			slot = (slot + 1) & (capacity - 1);
		if (!table[slot]) {
			table[slot] = i + 1;
			distinct++;
		}
	}
	free(table);
	return distinct;
}

static size_t
calibrate(const Rule *rules, size_t count, const char *s)
{
	Edit *edits = NULL;
	const char *p;
	size_t *next, n = strlen(s), r, k = 0, capacity = 0, i, distinct = 0;
	int ready;

	next = ecalloc(n + 1, sizeof(*next));
	for (r = 0; r < count; r++) {
		ready = 0;
		for (p = s; (p = strstr(p, rules[r].from)); p++) {
			if (k == capacity) {
				if (capacity > SIZE_MAX / 2)
					die("too many calibration edits");
				capacity = capacity ? capacity * 2 : 64;
				edits = ereallocarray(edits, capacity,
				                      sizeof(*edits));
			}
			edits[k++] = normalize(&rules[r], s, n,
			                       (size_t)(p - s), next, &ready);
		}
	}
	distinct = hashcount(edits, k);
	for (i = 0; i < k; i++)
		free(edits[i].text);
	free(edits);
	free(next);
	return distinct;
}

static long long
weight(const char *s)
{
	long long value = 0;
	size_t width;

	while (*s) {
		width = 1;
		if (*s >= 'A' && *s <= 'Z' && s[1] >= 'a' && s[1] <= 'z')
			width = 2;
		if (width == 2 && ((s[0] == 'R' && s[1] == 'n') ||
		                   (s[0] == 'A' && s[1] == 'r'))) {
			s += width;
			continue;
		}
		value += width == 1 && *s == 'Y' ? -1 : 1;
		s += width;
	}
	return value;
}

static int
invariant(const Rule *rules, size_t count, long long *electron)
{
	long long w;
	size_t r;

	*electron = LLONG_MAX;
	for (r = 0; r < count; r++) {
		if (strchr(rules[r].to, 'e'))
			return 0;
		w = weight(rules[r].to);
		if (strcmp(rules[r].from, "e") == 0) {
			if (*electron != LLONG_MAX && *electron != w - 1)
				return 0;
			*electron = w - 1;
		} else if (w - weight(rules[r].from) != 1) {
			return 0;
		}
	}
	return *electron != LLONG_MAX;
}

static size_t
fabricate(const Rule *rules, size_t count, const char *s)
{
	char names[256][64] = {{0}};
	Grammar g;
	size_t nnames = 0, *tokens;
	long long electron, answer;

	if (!invariant(rules, count, &electron))
		return part2(rules, count, s);
	tokens = ecalloc(strlen(s) + 1, sizeof(*tokens));
	(void)tokenize(s, names, &nnames, tokens, strlen(s) + 1);
	g = makegrammar(rules, count, names, &nnames);
	free(g.rules);
	free(tokens);
	if (strcmp(s, "e") == 0)
		return 0;
	answer = weight(s) - electron;
	if (answer < 0)
		return part2(rules, count, s);
	return (size_t)answer;
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
	           data.molecule
	               ? calibrate(data.rules, data.count, data.molecule)
	               : 0,
	           data.molecule
	               ? fabricate(data.rules, data.count, data.molecule)
	               : 0) < 0)
		die("printf:");
	free(data.rules);
	free(in);
	return EXIT_SUCCESS;
}
