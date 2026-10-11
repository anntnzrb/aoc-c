/* 2015 day 11, 01 optimal: https://adventofcode.com/2015/day/11
 *
 * approach: search allowed letters in lexical order, carrying straight and
 * pair state through prefixes; prune impossible pairs and short straights.
 * cost: time O(n), memory O(n), one input pass; fixed alphabet
 * bound: reading and emitting n letters costs Omega(n). Any free suffix
 * of five letters can be aabcc, so failed free searches have bounded depth.
 * rejected: bench 11 50 on Linux: work 312183 (312182-312183, n=5);
 * forbidden-range skipping 203089552 (203089550-203089557);
 * recursive prefix search 6904758 without straight pruning, 572070 with
 * it (572069-572070); recursion also fails clang-tidy misc-no-recursion.
 * 00 work 201997698 (201997696-201997701). Winner wall med 607.3 us,
 * CPU med 474.0 us, RSS 5672 KiB; shared-host wall times are noisy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	int straight, pairs, used, greater;
	char first;
} State;

static int allowed(int c);
static int search(char *password, const char *lower, int greater);

static int
allowed(int c)
{
	return c <= 'z' && c != 'i' && c != 'o' && c != 'l';
}

static int
search(char *password, const char *lower, int greater)
{
	State states[9] = {{0}}, *state, *next;
	int letters[8], c;
	size_t pos = 0;

	states[0].greater = greater;
enter:
	state = &states[pos];
	if (pos == 8) {
		if (state->greater && state->straight && state->pairs == 2)
			return 1;
		goto backtrack;
	}
	if (state->pairs + (int)((8 - pos + (pos && !state->used)) / 2) < 2)
		goto backtrack;
	if (!state->straight && pos > 5 &&
	    !(password[pos - 2] + 1 == password[pos - 1] &&
	      allowed(password[pos - 1] + 1)) &&
	    !(pos == 6 && allowed(password[pos - 1] + 1) &&
	      allowed(password[pos - 1] + 2)))
		goto backtrack;
	letters[pos] = state->greater ? 'a' : lower[pos];
advance:
	if (letters[pos] > 'z')
		goto backtrack;
	c = letters[pos]++;
	if (!allowed(c))
		goto advance;
	password[pos] = (char)c;
	state = &states[pos];
	next = &states[pos + 1];
	*next = *state;
	next->greater = state->greater || c > lower[pos];
	if (pos >= 2 && password[pos - 2] + 1 == password[pos - 1] &&
	    password[pos - 1] + 1 == c)
		next->straight = 1;
	next->used = 0;
	if (pos && password[pos - 1] == c && !state->used) {
		next->used = 1;
		if (!next->first) {
			next->first = (char)c;
			next->pairs = 1;
		} else if (next->first != c) {
			next->pairs = 2;
		}
	}
	++pos;
	goto enter;
backtrack:
	if (!pos)
		return 0;
	--pos;
	goto advance;
}

int
main(void)
{
	char *in, lower[9];
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
		memcpy(lower, in, sizeof(lower));
		if (!search(in, lower, 0) && !search(in, lower, 1))
			die("no valid password");
		if (printf("%s\n", in) < 0)
			die("printf:");
	}
	free(in);
	return EXIT_SUCCESS;
}
