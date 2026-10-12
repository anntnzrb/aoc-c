/* 2015 day 14, 01 optimal: https://adventofcode.com/2015/day/14
 *
 * approach: fill each racer's distance timeline by fly/rest blocks; merge
 *           maxima, then count ties using the saved timelines.
 * cost: time O(l+n*t), memory O(l+n*t), two input passes; t = 2503 seconds
 * bound: reading l bytes and n racers requires Omega(l+n). With fixed t,
 *        this matches the bound; variable-t optimality is not claimed.
 * rejected: just bench 14 50 on Linux, release, startup-subtracted work:
 *           naive 488029; phase intersections 509844; countdown 410070;
 *           regenerated timelines 182004; cached timelines 142361 winner
 *           (142360-142362, n=5). Caching trades O(n*t) storage for work.
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
timeline(const Reindeer *deer, int *row)
{
	int t = 0, distance = 0;
	while (t < SECONDS) {
		int start = t;
		int end = deer->fly < SECONDS - t ? t + deer->fly : SECONDS;
		for (int k = t; k < end; k++)
			row[k] = distance + ((k - start + 1) * deer->speed);
		distance += (end - start) * deer->speed;
		t = end;
		end = deer->rest < SECONDS - t ? t + deer->rest : SECONDS;
		for (int k = t; k < end; k++)
			row[k] = distance;
		t = end;
	}
}

static void
race(Reindeer *deer, size_t n)
{
	size_t i;
	int t, lead[2503], *rows, *row;

	if (!n)
		return;
	memset(lead, 0, sizeof(lead));
	rows = ecalloc(n, 2503 * sizeof(*rows));
	for (i = 0; i < n; i++) {
		row = rows + (i * 2503);
		timeline(&deer[i], row);
		deer[i].distance = row[2502];
		for (t = 0; t < 2503; t++)
			if (row[t] > lead[t])
				lead[t] = row[t];
	}
	for (i = 0; i < n; i++) {
		row = rows + (i * 2503);
		for (t = 0; t < 2503; t++)
			deer[i].points += row[t] == lead[t];
	}
	free(rows);
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
