/* 2015 day 20, 00 naive: https://adventofcode.com/2015/day/20
 *
 * approach: simulate each elf delivering to every multiple of its number;
 *           its own elf guarantees a house at ceil(target / 10).
 * cost: O(B log B) time, O(B) memory, one delivery pass and one search,
 *       where B = ceil(target / 10).
 * trade: stores the whole bounded street rather than factoring each house.
 */
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t part1(size_t target);

static size_t
part1(size_t target)
{
	size_t *presents, bound, elf, house, answer;

	bound = target / 10 + (target % 10 != 0);
	presents = ecalloc(bound + 1, sizeof(*presents));
	for (elf = 1; elf <= bound; elf++)
		for (house = elf; house <= bound; house += elf)
			presents[house] += elf;
	for (answer = 1; answer <= bound; answer++)
		if (presents[answer] >= bound)
			break;
	free(presents);
	return answer;
}

int
main(void)
{
	char *in;
	size_t len, target, answer = 0;

	in = slurp(stdin, &len);
	if (len) {
		if (sscanf(in, "%zu", &target) != 1 || !target)
			die("invalid present target");
		answer = part1(target);
	}
	if (printf("%zu\n0\n", answer) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
