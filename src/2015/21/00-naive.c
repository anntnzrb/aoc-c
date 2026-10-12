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
#include <stdbit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	int cost, damage, armor;
} Item;

static constexpr Item weapons[] = {
    {8, 4, 0}, {10, 5, 0}, {25, 6, 0}, {40, 7, 0}, {74, 8, 0}};
static constexpr Item armors[] = {{0, 0, 0},  {13, 0, 1}, {31, 0, 2},
                                  {53, 0, 3}, {75, 0, 4}, {102, 0, 5}};
static constexpr Item rings[] = {{25, 1, 0}, {50, 2, 0}, {100, 3, 0},
                                 {20, 0, 1}, {40, 0, 2}, {80, 0, 3}};

static int
stat(char **in, const char *label)
{
	size_t len = strlen(label);
	if (strncmp(*in, label, len) != 0)
		die("invalid boss stat label");
	const char *p = *in + len;
	char *end;
	errno = 0;
	long value = strtol(p, &end, 10);
	if (errno || p == end || value < 0 || value > INT_MAX ||
	    (*end != '\n' && *end != '\0'))
		die("invalid boss stat value");
	*in = end + (*end == '\n');
	return (int)value;
}

static Item
loadout(size_t w, size_t a, unsigned int mask)
{
	if (stdc_count_ones(mask) > 2)
		return (Item){-1, 0, 0};
	Item item = weapons[w];

	item.cost += armors[a].cost;
	item.armor += armors[a].armor;
	for (size_t r = 0; r < sizeof rings / sizeof *rings; r++)
		if (mask & (1U << r)) {
			item.cost += rings[r].cost;
			item.damage += rings[r].damage;
			item.armor += rings[r].armor;
		}
	return item;
}

static bool
wins(int hp, int damage, int armor, int attack, int defense)
{
	int player = 100;

	attack -= armor;
	damage -= defense;
	if (attack < 1)
		attack = 1;
	if (damage < 1)
		damage = 1;
	/* Stats <= INT_MAX and shop damage <= 14 keep subtraction in int. */
	while (hp > attack) {
		hp -= attack;
		player -= damage;
		if (player <= 0)
			return false;
	}
	return true;
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len);
	if (!len) {
		free(in);
		return EXIT_SUCCESS;
	}
	char *p = in;
	int hp = stat(&p, "Hit Points: ");
	int damage = stat(&p, "Damage: ");
	int armor = stat(&p, "Armor: ");
	if (!hp)
		die("boss hit points must be positive");
	free(in);
	int best = INT_MAX, worst = 0;
	for (size_t w = 0; w < sizeof weapons / sizeof *weapons; w++)
		for (size_t a = 0; a < sizeof armors / sizeof *armors; a++)
			for (unsigned int mask = 0; mask < 64; mask++) {
				Item item = loadout(w, a, mask);
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
