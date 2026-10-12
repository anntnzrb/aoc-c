/* 2015 day 05, 00 naive: https://adventofcode.com/2015/day/5
 *
 * approach: check each line for three vowels, a repeated adjacent letter,
 * and no forbidden substring for part 1. For part 2, compare every pair
 * with later nonoverlapping pairs and look for a letter repeating at gap two.
 * cost: part 1 O(N), part 2 O(N L), memory O(L), separate rule scans,
 * N input bytes, L longest line
 * trade: separate scans express each rule directly; getline buffers one line
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static bool
nice(const char *s)
{
	size_t vowels = 0;
	for (const char *p = s; *p; p++)
		if (strchr("aeiou", *p))
			vowels++;
	bool twice = false;
	for (const char *p = s; *p; p++)
		if (*p == p[1])
			twice = true;
	return vowels >= 3 && twice && !strstr(s, "ab") && !strstr(s, "cd") &&
	       !strstr(s, "pq") && !strstr(s, "xy");
}

static bool
nice2(const char *s)
{
	size_t len = strlen(s);
	bool pair = false;
	for (size_t i = 0; i + 1 < len && !pair; i++)
		for (size_t j = i + 2; j + 1 < len; j++)
			if (s[i] == s[j] && s[i + 1] == s[j + 1]) {
				pair = true;
				break;
			}
	bool gap = false;
	for (size_t i = 0; i + 2 < len; i++)
		if (s[i] == s[i + 2])
			gap = true;
	return pair && gap;
}

int
main(void)
{
	char *line = NULL;
	size_t cap = 0;
	long long count = 0, count2 = 0;
	while (getline(&line, &cap, stdin) >= 0) {
		int one = nice(line), two = nice2(line);
		if (ckd_add(&count, count, one) ||
		    ckd_add(&count2, count2, two))
			die("answer overflow");
	}
	if (ferror(stdin))
		die("getline:");
	free(line);
	if (printf("%lld\n%lld\n", count, count2) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
