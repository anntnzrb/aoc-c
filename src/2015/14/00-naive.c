/* 2015 day 14, 00 naive: https://adventofcode.com/2015/day/14
 *
 * approach: simulate each reindeer flying or resting each race second,
 *           then take the greatest distance after 2503 seconds.
 * cost: time O(n*t), memory O(1), one input pass; t = 2503 seconds
 * trade: directly follow the race rules instead of skipping whole cycles
 */
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

int
main(void)
{
	char *in, *p;
	size_t len;
	int speed, fly, rest, used, t, distance, best = 0;

	in = slurp(stdin, &len);
	for (p = in; *p; p += used) {
		if (sscanf(p,
		           "%*s can fly %d km/s for %d seconds, but then must "
		           "rest for %d seconds. %n",
		           &speed, &fly, &rest, &used) != 3 ||
		    speed < 0 || fly <= 0 || rest <= 0)
			die("invalid reindeer");
		distance = 0;
		for (t = 0; t < 2503; t++)
			if (t % (fly + rest) < fly)
				distance += speed;
		if (distance > best)
			best = distance;
	}
	if (printf("%d\n", best) < 0)
		die("printf:");
	free(in);
	return EXIT_SUCCESS;
}
