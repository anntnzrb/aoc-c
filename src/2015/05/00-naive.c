/* 2015 day 05, 00 naive: https://adventofcode.com/2015/day/5
 *
 * approach: check each line for three vowels, a repeated adjacent letter,
 * and the absence of each forbidden substring; count lines passing all three.
 * cost: time O(N), memory O(L), up to six scans per line, N input bytes,
 * L longest line
 * trade: separate scans express each rule directly; getline buffers one line
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static int nice(const char *s);

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

int
main(void)
{
	char *line;
	size_t cap;
	long long count;

	line = NULL;
	cap = 0;
	count = 0;
	while (getline(&line, &cap, stdin) >= 0)
		count += nice(line);
	if (ferror(stdin))
		die("getline:");
	free(line);
	if (printf("%lld\n", count) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
