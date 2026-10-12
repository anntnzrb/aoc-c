/* 2015 day 14, 00 naive: https://adventofcode.com/2015/day/14
 *
 * approach: simulate flying and resting each second; award a point to
 *           every reindeer tied for the lead after all have moved.
 * cost: time O(n*t), memory O(n), two input passes; t = 2503 seconds
 * trade: directly follow the race rules instead of skipping whole cycles
 */
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static constexpr int SECONDS = 2503;

typedef struct {
	int speed, fly, rest, distance, points;
} Reindeer;

static int
number(char **p)
{
	while (**p && (**p < '0' || **p > '9'))
		(*p)++;
	if (!**p)
		die("missing reindeer number");
	errno = 0;
	long value = strtol(*p, p, 10);
	if (errno || value > INT_MAX)
		die("invalid reindeer number");
	return (int)value;
}

static void
validate(const Reindeer *deer)
{
	if (!deer->fly || !deer->rest)
		die("invalid reindeer duration");
	int cycle, distance;
	if (ckd_add(&cycle, deer->fly, deer->rest))
		die("duration overflow");
	int flying =
	    ((SECONDS / cycle) * deer->fly) + MIN(SECONDS % cycle, deer->fly);
	if (ckd_mul(&distance, deer->speed, flying))
		die("distance overflow");
}

static void
race(Reindeer *deer, size_t n)
{
	for (int t = 0; n && t < SECONDS; t++) {
		int lead = 0;
		for (size_t i = 0; i < n; i++) {
			if (t % (deer[i].fly + deer[i].rest) < deer[i].fly)
				deer[i].distance += deer[i].speed;
			if (deer[i].distance > lead)
				lead = deer[i].distance;
		}
		for (size_t i = 0; i < n; i++)
			if (deer[i].distance == lead)
				deer[i].points++;
	}
}

int
main(void)
{
	Reindeer *deer;
	char *in, *p, *line, *save;
	size_t len, capacity = 1, n = 0, i;
	int distance = 0, points = 0;

	in = slurp(stdin, &len);
	for (p = in; *p; p++)
		if (*p == '\n')
			capacity++;
	deer = ecalloc(capacity, sizeof(*deer));
	for (line = strtok_r(in, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		p = line;
		deer[n].speed = number(&p);
		deer[n].fly = number(&p);
		deer[n].rest = number(&p);
		if (!deer[n].fly || !deer[n].rest)
			die("invalid reindeer duration");
		n++;
	}
	race(deer, n);
	for (i = 0; i < n; i++) {
		if (deer[i].distance > distance)
			distance = deer[i].distance;
		if (deer[i].points > points)
			points = deer[i].points;
	}
	if (printf("%d\n%d\n", distance, points) < 0)
		die("printf:");
	free(deer);
	free(in);
	return EXIT_SUCCESS;
}
