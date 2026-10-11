/* 2015 day 21, 01 optimal: https://adventofcode.com/2015/day/21
 *
 * approach: enumerate the 22 legal ring sets with each weapon and armor;
 *           precompute winning damage thresholds for all armor totals.
 * cost: O(B + W A R^2 + D) time, O(B + R^2 + D) space, one input pass;
 *       B bytes, W weapons, A armor choices, R rings, D armor totals.
 *       The fixed shop gives 660 loadouts, 22 ring sets, 11 armor totals.
 * bound: the fixed shop makes search constant; reading B bytes costs
 *        Omega(B), matched here. No combat-turn-dependent work remains.
 * rejected: just bench 21 50 on Linux: 00 simulation 122813 work;
 *           ring pairs with two kill-turn divisions per loadout 24717;
 *           armor thresholds win with 17147 (17147-17148, n=5).
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	int cost, damage, armor;
} Item;

static int stat(char **in, const char *label);
static void ringpairs(Item pairs[22]);
static void thresholds(long long need[11], int hp, int damage, int armor);

static const Item weapons[] = {
    {8, 4, 0}, {10, 5, 0}, {25, 6, 0}, {40, 7, 0}, {74, 8, 0}};
static const Item armors[] = {{0, 0, 0},  {13, 0, 1}, {31, 0, 2},
                              {53, 0, 3}, {75, 0, 4}, {102, 0, 5}};
static const Item rings[] = {{0, 0, 0},  {25, 1, 0}, {50, 2, 0}, {100, 3, 0},
                             {20, 0, 1}, {40, 0, 2}, {80, 0, 3}};

static int
stat(char **in, const char *label)
{
	const char *p;
	char *end;
	long value;
	size_t len;

	len = strlen(label);
	if (strncmp(*in, label, len) != 0)
		die("invalid boss stat label");
	p = *in + len;
	errno = 0;
	value = strtol(p, &end, 10);
	if (errno || p == end || value < 0 || value > INT_MAX ||
	    (*end != '\n' && *end != '\0'))
		die("invalid boss stat value");
	*in = end + (*end == '\n');
	return (int)value;
}

static void
ringpairs(Item pairs[22])
{
	size_t i, j, n = 0;

	for (i = 0; i < sizeof rings / sizeof *rings; i++)
		for (j = i; j < sizeof rings / sizeof *rings; j++) {
			if (i == j && i != 0)
				continue;
			pairs[n].cost = rings[i].cost + rings[j].cost;
			pairs[n].damage = rings[i].damage + rings[j].damage;
			pairs[n].armor = rings[i].armor + rings[j].armor;
			n++;
		}
}

static void
thresholds(long long need[11], int hp, int damage, int armor)
{
	int defense, hit, turns;

	for (defense = 0; defense < 11; defense++) {
		hit = damage - defense;
		if (hit < 1)
			hit = 1;
		turns = (99 / hit) + 1;
		/* Minimum damage already wins when the boss dies in time. */
		need[defense] =
		    hp <= turns ? 0
		                : (long long)armor + ((hp - 1) / turns) + 1;
	}
}

int
main(void)
{
	char *in, *p;
	Item pairs[22];
	long long need[11];
	size_t len, w, a, r;
	int hp, damage, armor, cost, attack, defense, best = INT_MAX,
	                                              worst = 0;

	in = slurp(stdin, &len);
	if (!len) {
		free(in);
		return EXIT_SUCCESS;
	}
	p = in;
	hp = stat(&p, "Hit Points: ");
	damage = stat(&p, "Damage: ");
	armor = stat(&p, "Armor: ");
	if (!hp)
		die("boss hit points must be positive");
	free(in);
	ringpairs(pairs);
	thresholds(need, hp, damage, armor);
	for (w = 0; w < sizeof weapons / sizeof *weapons; w++)
		for (a = 0; a < sizeof armors / sizeof *armors; a++)
			for (r = 0; r < sizeof pairs / sizeof *pairs; r++) {
				cost = weapons[w].cost + armors[a].cost +
				       pairs[r].cost;
				attack = weapons[w].damage + pairs[r].damage;
				defense = armors[a].armor + pairs[r].armor;
				if (attack >= need[defense]) {
					if (cost < best)
						best = cost;
				} else if (cost > worst) {
					worst = cost;
				}
			}
	if (best == INT_MAX)
		die("no winning loadout");
	if (printf("%d\n%d\n", best, worst) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
