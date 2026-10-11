/* 2015 day 21, 00 naive: https://adventofcode.com/2015/day/21
 *
 * approach: enumerate legal shop loadouts and simulate alternating attacks;
 *           keep the cheapest winning loadout.
 * cost: O(W A 2^R R + L T) time, O(1) space, one input pass; L loadouts,
 *       T combat turns, W weapons, A armor choices, R rings.
 * trade: direct combat simulation instead of calculating turns to kill.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

typedef struct {
	int cost, damage, armor;
} Item;

static int wins(int hp, int damage, int armor, int attack, int defense);

static const Item weapons[] = {
    {8, 4, 0}, {10, 5, 0}, {25, 6, 0}, {40, 7, 0}, {74, 8, 0}};
static const Item armors[] = {{0, 0, 0},  {13, 0, 1}, {31, 0, 2},
                              {53, 0, 3}, {75, 0, 4}, {102, 0, 5}};
static const Item rings[] = {{25, 1, 0}, {50, 2, 0}, {100, 3, 0},
                             {20, 0, 1}, {40, 0, 2}, {80, 0, 3}};

static int
wins(int hp, int damage, int armor, int attack, int defense)
{
	int player = 100;

	attack -= armor;
	damage -= defense;
	if (attack < 1)
		attack = 1;
	if (damage < 1)
		damage = 1;
	for (;;) {
		hp -= attack;
		if (hp <= 0)
			return 1;
		player -= damage;
		if (player <= 0)
			return 0;
	}
}

int
main(void)
{
	char *in;
	size_t len, w, a, r;
	unsigned int mask;
	int hp, damage, armor, cost, attack, defense, count, best = INT_MAX;

	in = slurp(stdin, &len);
	if (!len) {
		free(in);
		return EXIT_SUCCESS;
	}
	if (sscanf(in, "Hit Points: %d\nDamage: %d\nArmor: %d", &hp, &damage,
	           &armor) != 3 ||
	    hp <= 0 || damage < 0 || armor < 0)
		die("invalid boss stats");
	free(in);
	for (w = 0; w < sizeof weapons / sizeof *weapons; w++)
		for (a = 0; a < sizeof armors / sizeof *armors; a++)
			for (mask = 0; mask < 64; mask++) {
				cost = weapons[w].cost + armors[a].cost;
				attack = weapons[w].damage;
				defense = armors[a].armor;
				count = 0;
				for (r = 0; r < sizeof rings / sizeof *rings;
				     r++)
					if (mask & (1U << r)) {
						count++;
						cost += rings[r].cost;
						attack += rings[r].damage;
						defense += rings[r].armor;
					}
				if (count <= 2 &&
				    wins(hp, damage, armor, attack, defense) &&
				    cost < best)
					best = cost;
			}
	if (best == INT_MAX)
		die("no winning loadout");
	if (printf("%d\n", best) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
