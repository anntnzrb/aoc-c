/* 2015 day 20, 00 naive: https://adventofcode.com/2015/day/20
 *
 * approach: simulate elf deliveries to multiples, capped at 50 in part 2;
 *           its own elf guarantees a house at ceil(target / rate).
 * cost: O(B log B) time for part 1, O(50 B) for part 2, O(B) memory;
 *       one delivery pass and one search per part, B = ceil(target / rate).
 * trade: stores the whole bounded street rather than factoring each house.
 */
#include <errno.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static size_t
solve(size_t target, bool limited)
{
	size_t rate = limited ? 11 : 10;
	size_t bound = (target / rate) + (target % rate != 0);
	size_t *presents = ecalloc(bound + 1, sizeof(*presents));
	for (size_t elf = 1; elf <= bound; elf++)
		for (size_t house = elf, visit = 1;
		     house <= bound && (!limited || visit <= 50);
		     house += elf, visit++)
			if (ckd_add(&presents[house], presents[house], elf))
				die("present total too large");
	size_t answer;
	for (answer = 1; answer <= bound; answer++)
		if (presents[answer] >= bound)
			break;
	free(presents);
	return answer;
}

int
main(void)
{
	size_t len, part1 = 0, part2 = 0;
	char *in = slurp(stdin, &len);

	if (len) {
		errno = 0;
		char *end;
		size_t target = strtoul(in, &end, 10);
		if (errno || end == in || !target || *in == '-' ||
		    (*end && *end != '\n'))
			die("invalid present target");
		part1 = solve(target, false);
		part2 = solve(target, true);
	}
	if (printf("%zu\n%zu\n", part1, part2) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
