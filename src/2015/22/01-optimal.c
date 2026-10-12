/* 2015 day 22, 01 optimal: https://adventofcode.com/2015/day/22
 *
 * approach: A* over exact combat states, with an optimistic damage cost;
 *           discard repeated states and use circular priority buckets.
 * cost: expected O(V + E) time, O(V + E) memory, one input pass; V expanded
 *       states, E generated spell edges. Hash operations are amortized O(1).
 * bound: O(V + E) matches explicit traversal of the searched graph, E <= 5V;
 *        this does not prove the fewest possible states must be searched.
 *        Credit all pending poison ticks, then ceil(remaining * 173 / 18)
 *        lower-bounds spending: no spell buys more than 18 HP per 173 mana.
 *        This bound is consistent: a spell reduces it by at most its cost.
 *        First removed win is therefore cheapest. Priorities rise 0..229
 *        per edge, so 230 circular buckets replace heap comparisons.
 * rejected: uniform-cost buckets, 3,300,410 work vs A* 2,803,548; naive DFS
 *           183,248,510 work (Linux, just bench 22 50, both parts).
 */
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

enum { HP, MANA, BOSS, SHIELD, POISON, RECHARGE, NSTATS };
static constexpr size_t NBUCKETS = 230;

typedef struct {
	int v[NSTATS], spent;
} State;

typedef struct {
	State s;
	size_t next;
} Node;

typedef struct {
	State *table;
	Node *nodes;
	size_t size, used, capacity, count, live, heads[NBUCKETS];
	int priority;
} Search;

static void
effects(State *s)
{
	if (s->v[SHIELD])
		s->v[SHIELD]--;
	if (s->v[POISON]) {
		s->v[BOSS] -= 3;
		s->v[POISON]--;
	}
	if (s->v[RECHARGE]) {
		if (ckd_add(&s->v[MANA], s->v[MANA], 101))
			die("mana overflow");
		s->v[RECHARGE]--;
	}
}

static bool
player(State *s, bool hard)
{
	if (s->v[BOSS] <= 0)
		return true;
	s->v[HP] -= hard;
	if (s->v[HP] <= 0)
		return false;
	effects(s);
	return true;
}

static bool
cast(State *s, size_t spell, int damage)
{
	static constexpr int costs[] = {53, 73, 113, 173, 229};
	static constexpr int timers[] = {0, 0, 6, 6, 5};

	if (spell >= LEN(costs))
		return false;
	int cost = costs[spell];
	if (s->v[MANA] < cost)
		return false;
	size_t effect = spell + 1;
	if (spell >= 2 && s->v[effect])
		return false;
	s->v[MANA] -= cost;
	if (ckd_add(&s->spent, s->spent, cost))
		die("spending overflow");
	if (spell < 2) {
		s->v[BOSS] -= spell == 0 ? 4 : 2;
		if (spell == 1 && ckd_add(&s->v[HP], s->v[HP], 2))
			die("hit points overflow");
	} else {
		s->v[effect] = timers[spell];
	}
	if (s->v[BOSS] <= 0)
		return true;
	effects(s);
	if (s->v[BOSS] > 0)
		s->v[HP] -= MAX(1, damage - (s->v[SHIELD] ? 7 : 0));
	return s->v[HP] > 0;
}

static size_t
hash(State s, size_t size)
{
	uint64_t h = 0;
	for (size_t i = 0; i < NSTATS; i++)
		h = (h * 33 + (unsigned int)s.v[i]) & UINT32_MAX;
	h = ((h ^ (h >> 16)) * UINT64_C(0x45d9f3b)) & UINT32_MAX;
	return (size_t)(h ^ (h >> 16)) & (size - 1);
}

static void
grow(Search *q)
{
	State *old = q->table;
	size_t size = q->size;
	if (ckd_mul(&q->size, size, 2))
		die("table capacity overflow");
	if (!size)
		q->size = 1024;
	q->table = ecalloc(q->size, sizeof(*q->table));
	for (size_t i = 0; i < size; i++) {
		if (!old[i].v[HP])
			continue;
		size_t slot = hash(old[i], q->size);
		while (q->table[slot].v[HP])
			slot = (slot + 1) & (q->size - 1);
		q->table[slot] = old[i];
	}
	free(old);
}

static bool
remember(Search *q, State s)
{
	if (!q->size || q->used >= q->size / 2)
		grow(q);
	size_t slot = hash(s, q->size);
	while (q->table[slot].v[HP]) {
		if (memcmp(q->table[slot].v, s.v, sizeof(s.v)) == 0) {
			if (q->table[slot].spent <= s.spent)
				return false;
			q->table[slot].spent = s.spent;
			return true;
		}
		slot = (slot + 1) & (q->size - 1);
	}
	q->table[slot] = s;
	q->used++;
	return true;
}

static int
estimate(State s)
{
	int remaining = MAX(0, s.v[BOSS] - (3 * s.v[POISON]));
	long long cost = ((long long)remaining * 173 + 17) / 18;
	if (cost > INT_MAX)
		die("estimate overflow");
	return (int)cost;
}

static void
push(Search *q, State s)
{
	if (q->count == q->capacity) {
		if (ckd_mul(&q->capacity, q->capacity, 2))
			die("node capacity overflow");
		if (!q->capacity)
			q->capacity = 1024;
		q->nodes =
		    ereallocarray(q->nodes, q->capacity, sizeof(*q->nodes));
	}
	int priority;
	if (ckd_add(&priority, s.spent, estimate(s)))
		die("priority overflow");
	size_t bucket = (size_t)priority % NBUCKETS;
	q->nodes[q->count] = (Node){s, q->heads[bucket]};
	q->heads[bucket] = ++q->count;
	q->live++;
}

static State
pop(Search *q)
{
	size_t bucket = (size_t)q->priority % NBUCKETS;
	while (!q->heads[bucket]) {
		q->priority++;
		bucket = (size_t)q->priority % NBUCKETS;
	}
	size_t node = q->heads[bucket] - 1;
	q->heads[bucket] = q->nodes[node].next;
	q->live--;
	return q->nodes[node].s;
}

static int
fight(State initial, int damage, bool hard)
{
	Search q = {};
	int best = -1;

	q.priority = estimate(initial);
	push(&q, initial);
	while (q.live) {
		State s = pop(&q);
		if (!player(&s, hard))
			continue;
		if (s.v[BOSS] <= 0) {
			best = s.spent;
			break;
		}
		if (!remember(&q, s))
			continue;
		for (size_t spell = 0; spell < 5; spell++) {
			State next = s;
			if (cast(&next, spell, damage))
				push(&q, next);
		}
	}
	free(q.nodes);
	free(q.table);
	return best;
}

static int
stat(const char *in, const char *label, int fallback)
{
	const char *p = strstr(in, label);
	if (!p)
		return fallback;
	p += strlen(label);
	char *end;
	errno = 0;
	long value = strtol(p, &end, 10);
	if (errno || p == end || (*end && *end != '\n') || value < 0 ||
	    value > INT_MAX)
		die("invalid combat stat: %s", label);
	return (int)value;
}

static int
solve(const char *in, bool hard)
{
	if (!*in)
		return 0;
	State s = {};
	s.v[BOSS] = stat(in, "Hit Points: ", 0);
	int damage = stat(in, "Damage: ", 0);
	s.v[HP] = stat(in, "Player Hit Points: ", 50);
	s.v[MANA] = stat(in, "Player Mana: ", 500);
	if (s.v[BOSS] <= 0 || damage <= 0 || s.v[HP] <= 0)
		die("invalid combat stats");
	return fight(s, damage, hard);
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	if (printf("%d\n%d\n", solve(in, false), solve(in, true)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
