/* aoc.c: shared helpers for the Advent of Code solutions */
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#define CHUNK ((size_t)8192)

static int parsenum(const char *s, int *n);

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved = errno;
	size_t n;

	/* nothing sane left to do if stderr itself fails */
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);

	n = strlen(fmt);
	if (n > 0 && fmt[n - 1] == ':')
		(void)fprintf(stderr, " %s", strerror(saved));
	(void)fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

void *
ereallocarray(void *p, size_t nmemb, size_t size)
{
	void *q;

	if (size && nmemb > SIZE_MAX / size)
		die("reallocarray: %zu * %zu overflows", nmemb, size);
	if (!(q = realloc(p, nmemb * size)))
		die("realloc:");
	return q;
}

char *
slurp(FILE *fp, size_t *len)
{
	char *buf = NULL;
	size_t cap = 0, n = 0, r;

	do {
		if (cap - n < CHUNK) {
			if (cap > SIZE_MAX / 2)
				die("slurp: input too large");
			cap = cap ? cap * 2 : CHUNK;
			buf = ereallocarray(buf, cap, 1);
		}
		r = fread(buf + n, 1, cap - n - 1, fp);
		n += r;
	} while (r > 0);

	if (ferror(fp))
		die("fread:");
	buf[n] = '\0';
	*len = n;
	return buf;
}

/* a plain decimal of at most 9 digits, so it cannot overflow an int */
static int
parsenum(const char *s, int *n)
{
	size_t i;
	int v = 0;

	for (i = 0; s[i]; i++) {
		if (i == 9 || s[i] < '0' || s[i] > '9')
			return -1;
		v = (v * 10) + (s[i] - '0');
	}
	if (i == 0)
		return -1;
	*n = v;
	return 0;
}

/* AoC ran 25 days a year through 2024 and 12 from 2025 on */
const char *
puzzleday(const char *ys, const char *ds, int *year, int *day)
{
	int y, d;

	if (parsenum(ys, &y) < 0)
		return "year must be a number";
	if (y < 2015)
		return "year out of range";
	if (parsenum(ds, &d) < 0)
		return "day must be a number";
	if (d < 1 || d > (y < 2025 ? 25 : 12))
		return "day out of range";
	*year = y;
	*day = d;
	return NULL;
}
