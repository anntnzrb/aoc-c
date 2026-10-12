/* 2015 day 16, 00 naive: https://adventofcode.com/2015/day/16
 *
 * approach: filter remembered facts by equality for part 1, then by the
 *           four strict ranges and remaining equalities for part 2;
 *           absent facts are unknown, so they impose no constraint.
 * cost: time O(n) input bytes, memory O(longest line), one pass
 * trade: linear lookup in the fixed ten-property sample keeps parsing simple.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static long
number(const char *text, char **end)
{
	errno = 0;
	long value = strtol(text, end, 10);
	if (errno || *end == text || value < 0)
		die("invalid number");
	return value;
}

static void
match(char *fact, bool matches[2])
{
	static const char *const names[] = {
	    "children", "cats",     "samoyeds", "pomeranians", "akitas",
	    "vizslas",  "goldfish", "trees",    "cars",        "perfumes"};
	static constexpr long sample[] = {3, 7, 2, 3, 0, 0, 5, 3, 2, 1};

	fact += strspn(fact, " ");
	char *colon = strchr(fact, ':');
	if (!colon)
		die("invalid fact");
	*colon = 0;
	char *end;
	long value = number(colon + 1, &end);
	if (*end)
		die("trailing fact data");
	for (size_t i = 0; i < sizeof(sample) / sizeof(sample[0]); i++) {
		if (strcmp(fact, names[i]) != 0)
			continue;
		matches[0] &= value == sample[i];
		if (i == 1 || i == 7)
			matches[1] &= value > sample[i];
		else if (i == 3 || i == 6)
			matches[1] &= value < sample[i];
		else
			matches[1] &= value == sample[i];
		return;
	}
	die("unknown property: %s", fact);
}

int
main(void)
{
	char *line = NULL;
	size_t cap = 0;
	long answer[2] = {};

	while (getline(&line, &cap, stdin) != -1) {
		if (strncmp(line, "Sue ", 4) != 0)
			die("invalid Sue");
		char *end;
		long id = number(line + 4, &end);
		if (*end != ':' || !id)
			die("invalid Sue id");
		bool matches[2] = {true, true};
		for (char *fact = strtok(end + 1, ",\n"); fact;
		     fact = strtok(NULL, ",\n"))
			match(fact, matches);
		for (size_t i = 0; i < 2; i++)
			if (matches[i] && !answer[i])
				answer[i] = id;
	}
	if (ferror(stdin))
		die("read:");
	free(line);
	if (printf("%ld\n%ld\n", answer[0], answer[1]) < 0)
		die("write:");
	return EXIT_SUCCESS;
}
