/* 2015 day 16, 01 optimal: https://adventofcode.com/2015/day/16
 *
 * approach: read fixed blocks, dispatch the fixed property names, fuse both
 * comparisons, and skip remaining facts once neither part can match a Sue.
 * cost: time O(n) input bytes, memory O(1), one pass
 * bound: a deciding fact can be last, so worst-case time is Omega(n);
 *        fixed blocks and two answers attain the Omega(1) memory bound.
 * rejected: just bench 16 50 (Linux), work instructions: fgets rows
 *           212757, getline rows 191077, naive 1019072; blocks 147039.
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static long
number(char **cursor)
{
	char *p = *cursor;
	long value = 0;

	while (*p == ' ')
		p++;
	if (*p < '0' || *p > '9')
		die("invalid number");
	do {
		long digit = *p++ - '0';
		if (ckd_mul(&value, value, 10) ||
		    ckd_add(&value, value, digit))
			die("number too large");
	} while (*p >= '0' && *p <= '9');
	*cursor = p;
	return value;
}

static int
fact(char **cursor, int matches)
{
	char *p = *cursor;
	long ref;
	int direction = 0;

	switch (*p) {
	case 'a':
	case 'v':
		ref = 0;
		break;
	case 's':
		ref = 2;
		break;
	case 'g':
		ref = 5;
		direction = -1;
		break;
	case 't':
		ref = 3;
		direction = 1;
		break;
	case 'p':
		ref = p[1] == 'e' ? 1 : 3;
		direction = p[1] == 'e' ? 0 : -1;
		break;
	case 'c':
		ref = p[2] == 'r' ? 2 : 7;
		if (p[1] == 'h')
			ref = 3;
		direction = p[2] == 't';
		break;
	default:
		die("unknown property");
	}
	char *colon = strchr(p, ':');
	if (!colon)
		die("invalid fact");
	p = colon + 1;
	long value = number(&p);
	bool ranged;
	if (direction > 0)
		ranged = value > ref;
	else if (direction < 0)
		ranged = value < ref;
	else
		ranged = value == ref;
	*cursor = p;
	return matches & ((value == ref) | (ranged << 1));
}

static void
row(char *p, long answer[2])
{
	int matches = 3;

	if (strncmp(p, "Sue ", 4) != 0)
		die("invalid Sue");
	p += 4;
	long id = number(&p);
	if (*p++ != ':' || !id)
		die("invalid Sue id");
	while (matches && *p && *p != '\n') {
		while (*p == ' ' || *p == ',')
			p++;
		matches = fact(&p, matches);
	}
	if ((matches & 1) && !answer[0])
		answer[0] = id;
	if ((matches & 2) && !answer[1])
		answer[1] = id;
}

int
main(void)
{
	char buffer[4096];
	size_t used = 0, got;
	long answer[2] = {};

	while ((
	    got = fread(buffer + used, 1, sizeof(buffer) - used - 1, stdin))) {
		used += got;
		if (used < sizeof(buffer)) {
			buffer[used] = 0;
			char *p = buffer, *newline;
			while ((newline = memchr(
			            p, '\n', used - (size_t)(p - buffer)))) {
				*newline = 0;
				row(p, answer);
				p = newline + 1;
			}
			size_t left = used - (size_t)(p - buffer);
			memmove(buffer, p, left);
			used = left;
			if (used == sizeof(buffer) - 1)
				die("Sue row too long");
		} else
			die("input block too large");
	}
	if (ferror(stdin))
		die("read:");
	if (used && used < sizeof(buffer)) {
		buffer[used] = 0;
		row(buffer, answer);
	}
	if (printf("%ld\n%ld\n", answer[0], answer[1]) < 0)
		die("write:");
	return EXIT_SUCCESS;
}
