/* 2015 day 16, 00 naive: https://adventofcode.com/2015/day/16
 *
 * approach: reject each Sue whose remembered facts differ from the sample;
 *           absent facts are unknown, so they impose no constraint.
 * cost: time O(n) input bytes, memory O(longest line), one pass
 * trade: linear lookup in the fixed ten-property sample keeps parsing simple.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

int
main(void)
{
	static const char *const names[] = {
	    "children", "cats",     "samoyeds", "pomeranians", "akitas",
	    "vizslas",  "goldfish", "trees",    "cars",        "perfumes"};
	static const int sample[] = {3, 7, 2, 3, 0, 0, 5, 3, 2, 1};
	char *line = NULL, *fact, name[32];
	size_t cap = 0, i;
	int id, offset, value, matches, answer = 0;

	while (getline(&line, &cap, stdin) != -1) {
		offset = 0;
		if (sscanf(line, "Sue %d: %n", &id, &offset) != 1 || !offset)
			die("invalid Sue");
		matches = 1;
		for (fact = strtok(line + offset, ",\n"); fact;
		     fact = strtok(NULL, ",\n")) {
			fact += strspn(fact, " ");
			if (sscanf(fact, "%31[^:]: %d", name, &value) != 2)
				die("invalid fact");
			for (i = 0; i < sizeof(sample) / sizeof(sample[0]);
			     i++)
				if (strcmp(name, names[i]) == 0)
					break;
			if (i == sizeof(sample) / sizeof(sample[0]))
				die("unknown property: %s", name);
			if (value != sample[i])
				matches = 0;
		}
		if (matches) {
			if (answer)
				die("multiple matching Sues");
			answer = id;
		}
	}
	if (ferror(stdin))
		die("read:");
	free(line);
	if (printf("%d\n", answer) < 0)
		die("write:");
	return EXIT_SUCCESS;
}
