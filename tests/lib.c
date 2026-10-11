/* lib.c: unit tests for lib/aoc.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

static void test_slurp_empty(void);
static void test_slurp_multichunk(void);
static void test_puzzleday(void);

static void
test_slurp_empty(void)
{
	FILE *fp;
	char *s;
	size_t len;

	if (!(fp = tmpfile()))
		die("tmpfile:");
	s = slurp(fp, &len);
	assert(len == 0);
	assert(s[0] == '\0');
	free(s);
	if (fclose(fp) == EOF)
		die("fclose:");
}

/* bigger than several growth steps, so every realloc path runs */
static void
test_slurp_multichunk(void)
{
	FILE *fp;
	char *s;
	size_t i, len, n = 100000;

	if (!(fp = tmpfile()))
		die("tmpfile:");
	for (i = 0; i < n; i++)
		if (fputc('a' + (int)(i % 26), fp) == EOF)
			die("fputc:");
	if (fseek(fp, 0, SEEK_SET) < 0)
		die("fseek:");

	s = slurp(fp, &len);
	assert(len == n);
	assert(strlen(s) == n);
	for (i = 0; i < n; i++)
		assert(s[i] == 'a' + (int)(i % 26));
	free(s);
	if (fclose(fp) == EOF)
		die("fclose:");
}

static void
test_puzzleday(void)
{
	int y = 0, d = 0;

	assert(!puzzleday("2015", "4", &y, &d) && y == 2015 && d == 4);
	assert(!puzzleday("2015", "08", &y, &d) && d == 8);
	assert(!puzzleday("2024", "25", &y, &d) && d == 25);
	assert(!puzzleday("2025", "12", &y, &d) && y == 2025 && d == 12);
	assert(puzzleday("2025", "13", &y, &d));
	assert(puzzleday("2015", "26", &y, &d));
	assert(puzzleday("2015", "0", &y, &d));
	assert(puzzleday("2014", "1", &y, &d));
	assert(puzzleday("", "1", &y, &d));
	assert(puzzleday("2015", "", &y, &d));
	assert(puzzleday("2015", "-1", &y, &d));
	assert(puzzleday("2015", "4x", &y, &d));
	assert(puzzleday("99999999999", "1", &y, &d));
}

int
main(void)
{
	test_slurp_empty();
	test_slurp_multichunk();
	test_puzzleday();
	puts("lib: ok");
	return EXIT_SUCCESS;
}
