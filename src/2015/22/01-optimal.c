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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

enum { HP, MANA, BOSS, SHIELD, POISON, RECHARGE, NSTATS };
enum { NBUCKETS = 230 };

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

static void effects(State *s);
static int player(State *s, int hard);
static int cast(State *s, size_t spell, int damage);
static size_t hash(State s, size_t size);
static void grow(Search *q);
static int remember(Search *q, State s);
static int estimate(State s);
static void push(Search *q, State s);
static State pop(Search *q);
static int fight(State initial, int damage, int hard);
static int stat(const char *in, const char *label, int fallback);
static int solve(const char *in, int hard);

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
		s->v[MANA] += 101;
		s->v[RECHARGE]--;
	}
}

static int
player(State *s, int hard)
{
	if (s->v[BOSS] <= 0)
		return 1;
	s->v[HP] -= hard;
	if (s->v[HP] <= 0)
		return 0;
	effects(s);
	return 1;
}

static int
cast(State *s, size_t spell, int damage)
{
	static const int costs[] = {53, 73, 113, 173, 229};
	static const int timers[] = {0, 0, 6, 6, 5};
	size_t effect;

	if (spell >= LEN(costs))
		return 0;
	if (s->v[MANA] < costs[spell])
		return 0;
	effect = spell + 1;
	if (spell >= 2 && s->v[effect])
		return 0;
	s->v[MANA] -= costs[spell];
	s->spent += costs[spell];
	if (spell < 2) {
		s->v[BOSS] -= spell == 0 ? 4 : 2;
		if (spell == 1)
			s->v[HP] += 2;
	} else {
		s->v[effect] = timers[spell];
	}
	if (s->v[BOSS] <= 0)
		return 1;
	effects(s);
	if (s->v[BOSS] > 0)
		s->v[HP] -= MAX(1, damage - (s->v[SHIELD] ? 7 : 0));
	return s->v[HP] > 0;
}

static size_t
hash(State s, size_t size)
{
	uint64_t h = 0;
	size_t i;

	for (i = 0; i < NSTATS; i++)
		h = (h * 33 + (unsigned int)s.v[i]) & UINT32_MAX;
	h = ((h ^ (h >> 16)) * UINT64_C(0x45d9f3b)) & UINT32_MAX;
	return (size_t)(h ^ (h >> 16)) & (size - 1);
}

static void
grow(Search *q)
{
	State *old;
	size_t i, slot, size;

	old = q->table;
	size = q->size;
	q->size = size ? size * 2 : 1024;
	q->table = ecalloc(q->size, sizeof(*q->table));
	for (i = 0; i < size; i++) {
		if (!old[i].v[HP])
			continue;
		slot = hash(old[i], q->size);
		while (q->table[slot].v[HP])
			slot = (slot + 1) & (q->size - 1);
		q->table[slot] = old[i];
	}
	free(old);
}

static int
remember(Search *q, State s)
{
	size_t slot;

	if (!q->size || q->used >= q->size / 2)
		grow(q);
	slot = hash(s, q->size);
	while (q->table[slot].v[HP]) {
		if (!memcmp(q->table[slot].v, s.v, sizeof(s.v))) {
			if (q->table[slot].spent <= s.spent)
				return 0;
			q->table[slot].spent = s.spent;
			return 1;
		}
		slot = (slot + 1) & (q->size - 1);
	}
	q->table[slot] = s;
	q->used++;
	return 1;
}

static int
estimate(State s)
{
	int remaining;

	remaining = MAX(0, s.v[BOSS] - (3 * s.v[POISON]));
	return (int)(((long long)remaining * 173 + 17) / 18);
}

static void
push(Search *q, State s)
{
	size_t bucket;

	if (q->count == q->capacity) {
		q->capacity = q->capacity ? q->capacity * 2 : 1024;
		q->nodes =
		    ereallocarray(q->nodes, q->capacity, sizeof(*q->nodes));
	}
	bucket = (size_t)(s.spent + estimate(s)) % NBUCKETS;
	q->nodes[q->count] = (Node){s, q->heads[bucket]};
	q->heads[bucket] = ++q->count;
	q->live++;
}

static State
pop(Search *q)
{
	size_t bucket, node;

	bucket = (size_t)q->priority % NBUCKETS;
	while (!q->heads[bucket]) {
		q->priority++;
		bucket = (size_t)q->priority % NBUCKETS;
	}
	node = q->heads[bucket] - 1;
	q->heads[bucket] = q->nodes[node].next;
	q->live--;
	return q->nodes[node].s;
}

static int
fight(State initial, int damage, int hard)
{
	Search q = {0};
	State s, next;
	size_t spell;
	int best = -1;

	q.priority = estimate(initial);
	push(&q, initial);
	while (q.live) {
		s = pop(&q);
		if (!player(&s, hard))
			continue;
		if (s.v[BOSS] <= 0) {
			best = s.spent;
			break;
		}
		if (!remember(&q, s))
			continue;
		for (spell = 0; spell < 5; spell++) {
			next = s;
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
	const char *p;
	char *end;
	long value;

	p = strstr(in, label);
	if (!p)
		return fallback;
	p += strlen(label);
	errno = 0;
	value = strtol(p, &end, 10);
	if (errno || p == end || (*end && *end != '\n') || value < 0 ||
	    value > INT_MAX)
		die("invalid combat stat: %s", label);
	return (int)value;
}

static int
solve(const char *in, int hard)
{
	State s = {0};
	int damage;

	if (!*in)
		return 0;
	s.v[BOSS] = stat(in, "Hit Points: ", 0);
	damage = stat(in, "Damage: ", 0);
	s.v[HP] = stat(in, "Player Hit Points: ", 50);
	s.v[MANA] = stat(in, "Player Mana: ", 500);
	if (s.v[BOSS] <= 0 || damage <= 0 || s.v[HP] <= 0)
		die("invalid combat stats");
	return fight(s, damage, hard);
}

int
main(void)
{
	char *in;
	size_t len;

	in = slurp(stdin, &len);
	if (printf("%d\n%d\n", solve(in, 0), solve(in, 1)) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
