/* 2015 day 20, 00 naive: https://adventofcode.com/2015/day/20
 *
 * approach: simulate elf deliveries to multiples, capped at 50 in part 2;
 *           its own elf guarantees a house at ceil(target / rate).
 * cost: O(B log B) time for part 1, O(50 B) for part 2, O(B) memory;
 *       one delivery pass and one search per part, B = ceil(target / rate).
 * trade: stores the whole bounded street rather than factoring each house.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t solve(size_t target, int limited);

static size_t
solve(size_t target, int limited)
{
	size_t *presents, rate, bound, elf, house, visit, answer;

	rate = limited ? 11 : 10;
	bound = (target / rate) + (target % rate != 0);
	presents = ecalloc(bound + 1, sizeof(*presents));
	for (elf = 1; elf <= bound; elf++)
		for (house = elf, visit = 1;
		     house <= bound && (!limited || visit <= 50);
		     house += elf, visit++)
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
	char *in, *end;
	size_t len, target, part1 = 0, part2 = 0;

	in = slurp(stdin, &len);
	if (len) {
		errno = 0;
		target = strtoul(in, &end, 10);
		if (errno || end == in || !target || *in == '-' ||
		    (*end && *end != '\n'))
			die("invalid present target");
		part1 = solve(target, 0);
		part2 = solve(target, 1);
	}
	if (printf("%zu\n%zu\n", part1, part2) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
