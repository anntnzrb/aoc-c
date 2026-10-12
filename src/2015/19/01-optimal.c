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
#include <stdckdint.h>
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
	size_t a = strlen(rule->from), b = strlen(rule->to);

	size_t limit = MIN(n, n - a + b);
	next[limit] = limit;
	for (size_t q = limit; q > 0;) {
		q--;
		next[q] = next[q + 1];
		size_t t;
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
          bool *ready)
{
	size_t a = strlen(rule->from), b = strlen(rule->to), out = n - a + b;
	size_t limit = MIN(n, out), first = pos;

	while (first < limit && first - pos < b &&
	       s[first] == rule->to[first - pos])
		first++;
	if (first < limit && first - pos == b) {
		if (!*ready) {
			shifts(rule, s, n, next);
			*ready = true;
		}
		first = next[first];
	}
	size_t suffix = MIN(n - pos - a, limit - first);
	/* The prefix cap prevents this scan from crossing before the edit. */
	while (suffix < limit - first &&
	       productchar(rule, s, pos, out - suffix - 1) ==
	           s[n - suffix - 1])
		suffix++;
	size_t bytes = out - first - suffix;
	Edit edit = {first, n - suffix - first, ecalloc(bytes + 1, 1)};
	for (size_t i = 0; i < bytes; i++)
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
	uint64_t h = (uint64_t)edit->position,
	         removed = (uint64_t)edit->removed;

	h ^= ((removed & (UINT64_MAX >> 17)) << 17) | (removed >> 47);
	for (const unsigned char *p = (const unsigned char *)edit->text; *p;
	     p++)
		h = (((h & (UINT64_MAX >> 5)) << 5) | (h >> 59)) ^ *p;
	return h ^ (h >> 32) ^ (h >> 16);
}

static size_t
hashcount(const Edit *edits, size_t count)
{
	if (!count)
		return 0;
	size_t capacity = 1;
	while (capacity <= count) {
		if (capacity > SIZE_MAX / 2)
			die("too many calibration keys");
		capacity *= 2;
	}
	if (capacity > SIZE_MAX / 2)
		die("too many calibration keys");
	size_t *table = ecalloc(capacity, 2 * sizeof(*table));
	capacity *= 2;
	size_t distinct = 0;
	for (size_t i = 0; i < count; i++) {
		size_t slot = (size_t)edithash(&edits[i]) & (capacity - 1);
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
	size_t n = strlen(s), k = 0, capacity = 0;

	size_t *next = ecalloc(n + 1, sizeof(*next));
	for (size_t r = 0; r < count; r++) {
		size_t a = strlen(rules[r].from), b = strlen(rules[r].to), out;
		if (a > n)
			continue;
		if (ckd_add(&out, n - a, b) || out == SIZE_MAX)
			die("replacement too large");
		bool ready = false;
		for (const char *p = s; (p = strstr(p, rules[r].from)); p++) {
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
	size_t distinct = hashcount(edits, k);
	for (size_t i = 0; i < k; i++)
		free(edits[i].text);
	free(edits);
	free(next);
	return distinct;
}

static long long
weight(const char *s)
{
	long long value = 0;

	while (*s) {
		size_t width = 1;
		if (*s >= 'A' && *s <= 'Z' && s[1] >= 'a' && s[1] <= 'z')
			width = 2;
		if (width == 2 && ((s[0] == 'R' && s[1] == 'n') ||
		                   (s[0] == 'A' && s[1] == 'r'))) {
			s += width;
			continue;
		}
		if (ckd_add(&value, value, width == 1 && *s == 'Y' ? -1 : 1))
			die("molecule weight too large");
		s += width;
	}
	return value;
}

static bool
invariant(const Rule *rules, size_t count, long long *electron)
{
	*electron = LLONG_MAX;
	for (size_t r = 0; r < count; r++) {
		if (strchr(rules[r].to, 'e'))
			return false;
		long long w = weight(rules[r].to), difference;
		if (strcmp(rules[r].from, "e") == 0) {
			if (ckd_sub(&difference, w, 1))
				die("molecule weight too large");
			if (*electron != LLONG_MAX && *electron != difference)
				return false;
			*electron = difference;
		} else {
			if (ckd_sub(&difference, w, weight(rules[r].from)))
				die("molecule weight too large");
			if (difference != 1)
				return false;
		}
	}
	return *electron != LLONG_MAX;
}

static size_t
fabricate(const Rule *rules, size_t count, const char *s)
{
	long long electron;

	if (!invariant(rules, count, &electron))
		return part2(rules, count, s);
	char names[256][64] = {};
	size_t nnames = 0;
	size_t *tokens = ecalloc(strlen(s) + 1, sizeof(*tokens));
	(void)tokenize(s, names, &nnames, tokens, strlen(s) + 1);
	Grammar g = makegrammar(rules, count, names, &nnames);
	free(g.rules);
	free(tokens);
	if (strcmp(s, "e") == 0)
		return 0;
	long long answer;
	if (ckd_sub(&answer, weight(s), electron))
		die("molecule weight too large");
	if (answer < 0)
		return part2(rules, count, s);
	return (size_t)answer;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	Input data = parse(in);
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
