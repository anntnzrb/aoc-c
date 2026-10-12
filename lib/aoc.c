/* aoc.c: shared helpers for the Advent of Code solutions */
#include <errno.h>
#include <stdarg.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static constexpr size_t CHUNK = 8192;

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved = errno;

	/* nothing sane left to do if stderr itself fails */
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);

	size_t n = strlen(fmt);
	if (n > 0 && fmt[n - 1] == ':')
		(void)fprintf(stderr, " %s", strerror(saved));
	(void)fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (!p)
		die("calloc:");
	return p;
}

void *
ereallocarray(void *p, size_t nmemb, size_t size)
{
	size_t bytes;
	if (ckd_mul(&bytes, nmemb, size))
		die("reallocarray: %zu * %zu overflows", nmemb, size);
	void *q = realloc(p, bytes);
	if (!q)
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
			if (ckd_mul(&cap, cap, 2))
				die("slurp: input too large");
			if (!cap)
				cap = CHUNK;
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

size_t
nameid(char names[][64], size_t *count, size_t capacity, const char *name)
{
	for (size_t i = 0; i < *count; i++)
		if (strcmp(names[i], name) == 0)
			return i;
	size_t len = strlen(name);
	if (*count >= capacity || len >= sizeof(names[0]))
		die("name table full or name too long");
	memcpy(names[*count], name, len + 1);
	return (*count)++;
}

bool
nextperm(size_t *order, size_t n)
{
	if (n < 2)
		return false;
	size_t i = n - 1;
	for (; i > 0 && order[i - 1] >= order[i]; i--)
		;
	if (i == 0)
		return false;
	size_t j = n - 1;
	for (; order[j] <= order[i - 1]; j--)
		;
	size_t tmp = order[i - 1];
	order[i - 1] = order[j];
	order[j] = tmp;
	for (j = n - 1; i < j; i++, j--) {
		tmp = order[i];
		order[i] = order[j];
		order[j] = tmp;
	}
	return true;
}

/* a plain decimal of at most 9 digits, so it cannot overflow an int */
static bool
parsenum(const char *s, int *n)
{
	size_t i;
	int v = 0;

	for (i = 0; s[i]; i++) {
		if (i == 9 || s[i] < '0' || s[i] > '9')
			return false;
		v = (v * 10) + (s[i] - '0');
	}
	if (i == 0)
		return false;
	*n = v;
	return true;
}

/* AoC ran 25 days a year through 2024 and 12 from 2025 on */
const char *
puzzleday(const char *ys, const char *ds, int *year, int *day)
{
	int y;
	if (!parsenum(ys, &y))
		return "year must be a number";
	if (y < 2015)
		return "year out of range";
	int d;
	if (!parsenum(ds, &d))
		return "day must be a number";
	if (d < 1 || d > (y < 2025 ? 25 : 12))
		return "day out of range";
	*year = y;
	*day = d;
	return NULL;
}
