/* 2015 day 22, 00 naive: https://adventofcode.com/2015/day/22
 *
 * approach: enumerate legal spell sequences, applying effects on each turn
 *           after the hard-mode player HP penalty;
 *           discard losses and spending no better than the cheapest win.
 * cost: O(5^d) time, O(d) DFS stack for explored spell depth d, one pass.
 * trade: direct combat simulation without memoizing repeated states.
 *        Optional Player Hit Points / Player Mana fields reproduce examples.
 */
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	int hp, mana, boss, shield, poison, recharge, spent;
} State;

static void
effects(State *s)
{
	if (s->shield)
		s->shield--;
	if (s->poison) {
		s->boss -= 3;
		s->poison--;
	}
	if (s->recharge) {
		if (ckd_add(&s->mana, s->mana, 101))
			die("mana overflow");
		s->recharge--;
	}
}

static bool
cast(State *s, size_t spell, int best)
{
	static constexpr int costs[] = {53, 73, 113, 173, 229};

	if (costs[spell] > s->mana || s->spent >= best - costs[spell])
		return false;
	if ((spell == 2 && s->shield) || (spell == 3 && s->poison) ||
	    (spell == 4 && s->recharge))
		return false;
	s->mana -= costs[spell];
	s->spent += costs[spell];
	switch (spell) {
	case 0:
		s->boss -= 4;
		break;
	case 1:
		s->boss -= 2;
		if (ckd_add(&s->hp, s->hp, 2))
			die("hit points overflow");
		break;
	case 2:
		s->shield = 6;
		break;
	case 3:
		s->poison = 6;
		break;
	case 4:
		s->recharge = 5;
		break;
	default:
		die("invalid spell");
	}
	return true;
}

static void
attack(State *s, int damage)
{
	/* An instant kill ends combat before the boss turn starts. */
	if (s->boss <= 0)
		return;
	effects(s);
	if (s->boss > 0)
		s->hp -= MAX(1, damage - (s->shield ? 7 : 0));
}

static int
fight(State initial, int damage, bool hard)
{
	size_t n = 1, cap = 256;
	int best = INT_MAX;

	State *stack = ecalloc(cap, sizeof(*stack));
	stack[0] = initial;
	while (n) {
		State s = stack[--n];
		if (s.spent >= best)
			continue;
		if (s.boss > 0) {
			s.hp -= hard;
			if (s.hp <= 0)
				continue;
			effects(&s);
		}
		if (s.boss <= 0) {
			best = s.spent;
			continue;
		}
		for (size_t spell = 0; spell < 5; spell++) {
			State next = s;
			if (!cast(&next, spell, best))
				continue;
			attack(&next, damage);
			if (next.hp <= 0)
				continue;
			if (n == cap) {
				cap *= 2;
				stack =
				    ereallocarray(stack, cap, sizeof(*stack));
			}
			stack[n++] = next;
		}
	}
	free(stack);
	return best == INT_MAX ? -1 : best;
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
	s.boss = stat(in, "Hit Points: ", 0);
	int damage = stat(in, "Damage: ", 0);
	s.hp = stat(in, "Player Hit Points: ", 50);
	s.mana = stat(in, "Player Mana: ", 500);
	if (s.boss <= 0 || damage <= 0 || s.hp <= 0)
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
