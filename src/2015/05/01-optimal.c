/* 2015 day 05, 01-optimal: https://adventofcode.com/2015/day/5
 *
 * approach: track vowels, doubles, forbidden pairs and gap-two repeats;
 * retain the earliest position of each byte pair to exclude overlaps.
 * cost: time O(N), memory O(L + 256^2), one scan per line,
 * N input bytes, L longest line; the pair table spans all lines
 * bound: any byte can change a rule or line boundary, requiring Omega(N)
 * time; a fused scan and constant-time pair lookup meet this bound.
 * rejected: just bench 5 50 on Linux: two getline scans 1128411 work,
 * fixed-buffer streaming 1225025 work vs fused getline 1103867 work;
 * streaming uses constant memory but loses on instructions
 */
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

static void
countline(const char *s, size_t len, size_t *first, size_t base,
          long long count[2])
{
	size_t vowels = 0;
	bool twice = false, forbidden = false, pair = false, gap = false;
	unsigned char prev = 0;
	for (size_t i = 0; i < len; i++) {
		unsigned char c = (unsigned char)s[i];
		if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
			vowels++;
		if (i) {
			twice |= c == prev;
			forbidden |= (prev == 'a' && c == 'b') ||
			             (prev == 'c' && c == 'd') ||
			             (prev == 'p' && c == 'q') ||
			             (prev == 'x' && c == 'y');
			size_t key = (((size_t)prev << 8) | c) & 65535U;
			size_t pos = base + i;
			/* Older lines have positions at or below base. */
			if (first[key] <= base)
				first[key] = pos;
			else
				pair |= pos - first[key] >= 2;
		}
		if (i >= 2)
			gap |= c == (unsigned char)s[i - 2];
		prev = c;
	}
	if (ckd_add(&count[0], count[0], vowels >= 3 && twice && !forbidden) ||
	    ckd_add(&count[1], count[1], pair && gap))
		die("answer overflow");
}

int
main(void)
{
	static size_t first[65536];
	char *line = NULL;
	size_t cap = 0, base = 0;
	ssize_t n;
	long long count[2] = {};
	while ((n = getline(&line, &cap, stdin)) >= 0) {
		size_t len = (size_t)n;
		if (len && line[len - 1] == '\n')
			len--;
		size_t next;
		if (ckd_add(&next, len, 1) || ckd_add(&next, base, next))
			die("position overflow");
		countline(line, len, first, base, count);
		base = next;
	}
	if (ferror(stdin))
		die("getline:");
	free(line);
	if (printf("%lld\n%lld\n", count[0], count[1]) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
