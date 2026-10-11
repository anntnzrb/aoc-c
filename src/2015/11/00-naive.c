/* 2015 day 11, 00 naive: https://adventofcode.com/2015/day/11
 *
 * approach: increment the password like base-26 counting until it has a
 * straight, no forbidden letters, and two different non-overlapping pairs.
 * cost: time O(k * n), memory O(n), one input pass; k candidates, n letters
 * trade: check every candidate directly instead of skipping invalid ranges
 */
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static void increment(char *password, size_t len);
static int valid(const char *password, size_t len);

static void
increment(char *password, size_t len)
{
	while (len) {
		--len;
		if (password[len] != 'z') {
			++password[len];
			return;
		}
		password[len] = 'a';
	}
}

static int
valid(const char *password, size_t len)
{
	size_t i;
	int straight = 0, pairs = 0;
	char first = '\0';

	for (i = 0; i < len; ++i) {
		if (password[i] == 'i' || password[i] == 'o' ||
		    password[i] == 'l')
			return 0;
		if (i >= 2 && password[i - 2] + 1 == password[i - 1] &&
		    password[i - 1] + 1 == password[i])
			straight = 1;
	}
	for (i = 0; i + 1 < len; ++i) {
		if (password[i] != password[i + 1])
			continue;
		if (!first) {
			first = password[i];
			pairs = 1;
		} else if (password[i] != first) {
			pairs = 2;
		}
		++i;
	}
	return straight && pairs == 2;
}

int
main(void)
{
	char *in;
	size_t len, i;

	in = slurp(stdin, &len);
	if (!len) {
		free(in);
		return EXIT_SUCCESS;
	}
	while (len && (in[len - 1] == '\n' || in[len - 1] == '\r'))
		in[--len] = '\0';
	if (len != 8)
		die("expected eight lowercase letters");
	for (i = 0; i < len; ++i)
		if (in[i] < 'a' || in[i] > 'z')
			die("expected eight lowercase letters");
	for (i = 0; i < 2; ++i) {
		do {
			increment(in, len);
		} while (!valid(in, len));
		if (printf("%s\n", in) < 0)
			die("printf:");
	}
	free(in);
	return EXIT_SUCCESS;
}
