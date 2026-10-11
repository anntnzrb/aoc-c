/* 2015 day 05, 00 naive: https://adventofcode.com/2015/day/5
 *
 * approach: check each line for three vowels, a repeated adjacent letter,
 * and no forbidden substring for part 1. For part 2, compare every pair
 * with later nonoverlapping pairs and look for a letter repeating at gap two.
 * cost: part 1 O(N), part 2 O(N L), memory O(L), separate rule scans,
 * N input bytes, L longest line
 * trade: separate scans express each rule directly; getline buffers one line
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static int nice(const char *s);
static int nice2(const char *s);

static int
nice(const char *s)
{
	const char *p;
	size_t vowels;
	int twice;

	vowels = 0;
	for (p = s; *p; p++)
		if (strchr("aeiou", *p))
			vowels++;
	twice = 0;
	for (p = s; *p; p++)
		if (*p == p[1])
			twice = 1;
	return vowels >= 3 && twice && !strstr(s, "ab") && !strstr(s, "cd") &&
	       !strstr(s, "pq") && !strstr(s, "xy");
}

static int
nice2(const char *s)
{
	size_t i, j, len;
	int pair, gap;

	len = strlen(s);
	pair = 0;
	for (i = 0; i + 1 < len && !pair; i++)
		for (j = i + 2; j + 1 < len; j++)
			if (s[i] == s[j] && s[i + 1] == s[j + 1]) {
				pair = 1;
				break;
			}
	gap = 0;
	for (i = 0; i + 2 < len; i++)
		if (s[i] == s[i + 2])
			gap = 1;
	return pair && gap;
}

int
main(void)
{
	char *line;
	size_t cap;
	long long count, count2;

	line = NULL;
	cap = 0;
	count = 0;
	count2 = 0;
	while (getline(&line, &cap, stdin) >= 0) {
		count += nice(line);
		count2 += nice2(line);
	}
	if (ferror(stdin))
		die("getline:");
	free(line);
	if (printf("%lld\n%lld\n", count, count2) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
