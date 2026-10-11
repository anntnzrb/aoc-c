/* 2015 day 21, 00 naive: https://adventofcode.com/2015/day/21
 *
 * approach: enumerate legal shop loadouts and simulate alternating attacks;
 *           keep the cheapest winner and the most expensive loser.
 * cost: O(W A 2^R R + L T) time, O(1) space, one input pass; L loadouts,
 *       T combat turns, W weapons, A armor choices, R rings.
 * trade: direct combat simulation instead of calculating turns to kill.
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
static Item loadout(size_t w, size_t a, unsigned int mask);
static int wins(int hp, int damage, int armor, int attack, int defense);

static const Item weapons[] = {
    {8, 4, 0}, {10, 5, 0}, {25, 6, 0}, {40, 7, 0}, {74, 8, 0}};
static const Item armors[] = {{0, 0, 0},  {13, 0, 1}, {31, 0, 2},
                              {53, 0, 3}, {75, 0, 4}, {102, 0, 5}};
static const Item rings[] = {{25, 1, 0}, {50, 2, 0}, {100, 3, 0},
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

static Item
loadout(size_t w, size_t a, unsigned int mask)
{
	Item item;
	size_t r;
	int count = 0;

	item = weapons[w];
	item.cost += armors[a].cost;
	item.armor += armors[a].armor;
	for (r = 0; r < sizeof rings / sizeof *rings; r++)
		if (mask & (1U << r)) {
			count++;
			item.cost += rings[r].cost;
			item.damage += rings[r].damage;
			item.armor += rings[r].armor;
		}
	if (count > 2)
		item.cost = -1;
	return item;
}

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
	char *in, *p;
	Item item;
	size_t len, w, a;
	unsigned int mask;
	int hp, damage, armor, best = INT_MAX, worst = 0;

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
	for (w = 0; w < sizeof weapons / sizeof *weapons; w++)
		for (a = 0; a < sizeof armors / sizeof *armors; a++)
			for (mask = 0; mask < 64; mask++) {
				item = loadout(w, a, mask);
				if (item.cost < 0)
					continue;
				if (wins(hp, damage, armor, item.damage,
				         item.armor)) {
					if (item.cost < best)
						best = item.cost;
				} else if (item.cost > worst) {
					worst = item.cost;
				}
			}
	if (best == INT_MAX)
		die("no winning loadout");
	if (printf("%d\n%d\n", best, worst) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
